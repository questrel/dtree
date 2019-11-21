// shared memory allocator -*- C++ -*-

// Copyright (C) 2019 Questrel, Inc.

/** @file shared_memory_allocator.h
 *  This file is an extension to the Standard C++ Library.
 */

#ifndef _SHARED_MEMORY_ALLOCATOR_H
#define _SHARED_MEMORY_ALLOCATOR_H 1

#include <cerrno> // for errno
#include <climits> // for CHAR_BIT
#include <cstddef> // for ptrdiff_t, size_t
#include <cstdint> // for uint32_t, etc.
#include <cstring> // for strerror
#include <fcntl.h> // for fcntl(), open(), and their flags
#include <memory> // for std::allocator
#include <stdexcept> // for std::runtime_error
#include <string> // for std::string
#include <sys/mman.h> // for mmap(), off_t

// for fcntl() on Linux prior to 3.15 and non-Linux systems prior to POSIX.1
#ifndef F_OFD_SETLKW
 #define F_OFD_SETLKW F_SETLKW
#endif
// NB. "OFD" stands for "open file descriptor".  From http://man7.org/linux/man-pages/man2/fcntl.2.html:
/* The principal difference between the two lock types is that whereas
       traditional record locks are associated with a process, open file
       description locks are associated with the open file description on
       which they are acquired, much like locks acquired with flock(2).
       Consequently (and unlike traditional advisory record locks), open
       file description locks are inherited across fork(2) (and clone(2)
       with CLONE_FILES), and are only automatically released on the last
       close of the open file description, instead of being released on any
       close of the file.
       */
// For more details see: https://gavv.github.io/articles/file-locks/#open-file-description-locks-fcntl

namespace questrel
{

  /**
   *  @brief  This is a power of 2 allocator which will maintain one global freelist
   *  for all processes in a memory mapped file.
   *
   */
  template<typename _Tp>
    class shared_memory_allocator : public std::allocator<_Tp>
    {
public:
      typedef typename std::allocator_traits<std::allocator<_Tp>>::value_type value_type;
      typedef typename std::allocator_traits<std::allocator<_Tp>>::size_type size_type;
      typedef typename std::allocator_traits<std::allocator<_Tp>>::pointer pointer;

private:
      // NB. assumes that largest block of shared memory is 2 to the power of number of bits in pointer
      static const size_type __max_index = sizeof(pointer) * CHAR_BIT;

      int __file_descriptor; // file descriptor of memory mapped file

      struct _block { // block of shared memory
	// NB. A data type is assumed to be aligned on a boundary that is the next power of 2 greater than its length.
	// For example, a double of 8 bytes is assumed to be aligned on an 8-byte boundary.
	_block* __next; // next block of memory in free list
      };

      struct _free_list_entry {
	_block* __base; // address of shared memory block for this power of 2
	// NB.  A free list may contain blocks based at higher powers of 2.
	_block* __first; // address of first block in free list for this power of 2
      };
      
      struct _header {
	// NB.  This file is not portable across systems with different pointer sizes.
	// NB.  This file is not portable across systems with different endians.
	uint32_t __header_size; // header size = 4 + sizeof(pointer) + sizeof(_free_list_entry) * __max_index
	pointer __root; // pointer to root element of data type _Tp
	_free_list_entry __free_list[__max_index];
      };

      _header* __header;

      void
	throw_error(const char *message)
      {
	throw(std::runtime_error(std::string("shared_memory_allocator unable to ") + message + " with error: " + strerror(errno)));
      }

      //
      // memory map each previously allocated block to address previously returned by mmap()
      //
      void
	init(const char* __file_name)
      {
	__file_descriptor = open(__file_name, O_RDWR | O_CREAT, 0660);
	if (__file_descriptor == -1)
	{
	  throw_error("open");
	}
	off_t __offset = lseek(__file_descriptor, 0, SEEK_END);
	if (__offset == -1) 
	{
	  throw_error("lseek");
	}
	if (__offset == 0) // file is empty
	{
	  lock_for_write();
	  __header = reinterpret_cast<_header*>(allocate_shared_memory_block(sizeof(_header)));
	  __offset = lseek(__file_descriptor, 0, SEEK_END);
	  if (__offset == -1) 
	  {
	    throw_error("lseek");
	  }
	  __header->__header_size = static_cast<uint32_t>(__offset);
	  unlock();
	}
	else
	{
	  __offset = lseek(__file_descriptor, 0, SEEK_SET);
	  if (__offset == -1) 
	  {
	    throw_error("lseek");
	  }
	  uint32_t __header_length;
	  if (sizeof(__header_length) != read(__file_descriptor, &__header_length, sizeof(__header_length)))
	  {
	    throw_error("read");
	  }
	  if (__header_length != shared_memory_header_size())
	  {
	    throw_error("invalid file size");
	  }
	  void* __v = mmap(0, __header_length, PROT_WRITE, MAP_SHARED, __file_descriptor, 0);
	  if (__v == MAP_FAILED)
	  {
	    throw_error("mmap");
	  }
	  __header = reinterpret_cast<_header*>(__v);
	}
	lock_for_read();
	__offset = __header->__header_size;
	for (size_type __index = 0; __index != __max_index; ++__index)
	{
	  if (__header->__free_list[__index].__base)
	  {
	    size_type __size = 1 << __index;
	    void* __v = mmap(__header->__free_list[__index].__base, __size, PROT_WRITE, MAP_SHARED, __file_descriptor, __offset);
	    if (__v == MAP_FAILED)
	    {
	      throw_error("mmap");
	    }
	    __offset += __size;
	  }
	}
	unlock();
      }

public:

      // NB. locks work on NFS mounted files in Linux kernels after 2.6.12
      void
	lock_for_read()
      {
	struct flock fl;
	fl.l_type = F_RDLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	if (fcntl(__file_descriptor, F_OFD_SETLKW, &fl) == -1) {
	  throw_error("fcntl");
	}
      }

      // NB. locks work on NFS mounted files in Linux kernels after 2.6.12
      void
	lock_for_write()
      {
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	if (fcntl(__file_descriptor, F_OFD_SETLKW, &fl) == -1) {
	  throw_error("fcntl");
	}
      }

      // NB. locks work on NFS mounted files in Linux kernels after 2.6.12
      void
	unlock()
      {
	struct flock fl;
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	if (fcntl(__file_descriptor, F_OFD_SETLKW, &fl) == -1) {
	  throw_error("fcntl");
	}
      }

      bool check_address(char *address) {
	return true; // TODO
      }

private:

      size_type
	minimum_shared_memory_block_size()
      {
	return static_cast<size_type>(sysconf(_SC_PAGE_SIZE));
      }

      uint32_t
	shared_memory_header_size()
      {
	return std::max<uint32_t>(sizeof(_header), minimum_shared_memory_block_size());
      }

      _block*
	allocate_shared_memory_block(size_type __bytes)
      {
	off_t __offset = lseek(__file_descriptor, 0, SEEK_END);
	if (__offset == -1) 
	{
	  throw_error("lseek");
	}
	// NB. mmap must be aligned on a page boundary
	__bytes = std::max<size_type>(__bytes, minimum_shared_memory_block_size());
	if (ftruncate(__file_descriptor, __offset + __bytes)) // extend file by __bytes bytes and clear them
	{
	  throw_error("ftruncate");
	}
	void *__v =  mmap(0, __bytes, PROT_WRITE, MAP_SHARED, __file_descriptor, __offset);
	if (__v == MAP_FAILED)
	{
	  throw_error("mmap");
	}
	return reinterpret_cast<_block*>(__v);
      }

      size_type
	ceiling_log_2(size_type __x)
      {
	size_type __retval = 0;
	for (size_type __y = 1; __y < __x; __y <<= 1)
	  ++__retval;
	return __retval;
      }


    template<class Any> friend class shared_memory_allocator;

public:

      shared_memory_allocator(const char* __file_name) { init(__file_name); }

      template<typename _Tp1>
        shared_memory_allocator(const shared_memory_allocator<_Tp1>& __s)
	{
          __file_descriptor = __s.__file_descriptor;
	  __header = reinterpret_cast<_header*>(__s.__header);
	}

      ~shared_memory_allocator() { }

      pointer
	allocate(size_type __n, const void* = 0);

      void
	deallocate(pointer __p, size_type __n);
      
      pointer
	get_root()
      {
	return __header->__root;
      }

      void
	set_root(pointer root)
      {
	__header->__root = root;
      }
    };

  template<typename _Tp>
    typename shared_memory_allocator<_Tp>::pointer
    shared_memory_allocator<_Tp>::
    allocate(size_type __n, const void*)
    {

      size_type __bytes = __n * sizeof(_Tp);
      size_type __log_2_size = ceiling_log_2(__bytes);
      size_type __index = __log_2_size;
      lock_for_write();
      _block* __retval = __header->__free_list[__index].__first; // block to return (if any)
      if (__retval)
      {
	__header->__free_list[__index].__first = __retval->__next; // unchain block to return from free list
      }
      else
      {
	while (!__header->__free_list[++__index].__first) // loop while free list is empty
	{ 
	  if (__index >= ceiling_log_2(minimum_shared_memory_block_size())) // if index is above minumum size
	  {
	    if (!__header->__free_list[__index].__base) // if block is not allocated
	    {
	      __header->__free_list[__index].__base = allocate_shared_memory_block(1 << __index); // allocate block
	      __header->__free_list[__index].__first = __header->__free_list[__index].__base; // chain block to (empty) free list
	      if (__header->__free_list[__index].__first->__next)
	      {
		throw_error("mmap is not zeroed at start");
	      }
	      break; // free list is not empty
	    }
	  }
	}
	__retval = __header->__free_list[__index].__first; // block to return after it is subdivided
	__header->__free_list[__index].__first = __retval->__next; // unchain block to return from free list
	// subdivide return block and chain second halves on all free list entries down to requested size
	while (--__index >= __log_2_size) // move to next lower power of 2 free list entry down to requested size
	{
	  if (__header->__free_list[__index].__first) // NB. all lower power of 2 free lists are empty
	  {
	    throw_error("lower free list is not empty");
	  }
	  // chain remainder of return block with length appropriate for this free list entry
	  __header->__free_list[__index].__first = reinterpret_cast<_block*>(reinterpret_cast<char*>(__retval) + (1 << __index));
	  if (__header->__free_list[__index].__first->__next)
	  {
	    throw_error("mmap is not zeroed in middle");
	  }
	}
      }
      unlock();
      return reinterpret_cast<_Tp*>(__retval);
    }
  
  template<typename _Tp>
    void
    shared_memory_allocator<_Tp>::
    deallocate(pointer __p, size_type __n)
    {
      if (__builtin_expect(__p != 0, true))
	{
	  size_t __bytes = __n * sizeof(_Tp);
	  size_type __index = ceiling_log_2(__bytes);
	  _block* __block_to_free = reinterpret_cast<_block*>(__p);
	  lock_for_write();
	  __block_to_free->__next = __header->__free_list[__index].__first;
	  __header->__free_list[__index].__first = __block_to_free;
	  unlock();
	}
    }
  
  template<typename _Tp>
    inline bool
    operator==(const shared_memory_allocator<_Tp>&, const shared_memory_allocator<_Tp>&)
    { return true; }
  
  template<typename _Tp>
    inline bool
    operator!=(const shared_memory_allocator<_Tp>&, const shared_memory_allocator<_Tp>&)
    { return false; }

} // namespace

#endif // _SHARED_MEMORY_ALLOCATOR_H
