#ifndef PERSISTENT_CONFIG__H
#define PERSISTENT_CONFIG__H

#include <eeprom.h>

class PersistentConfigItemBase;

// 
class PersistentConfig {
private:
  friend class PersistentConfigItemBase;
  PersistentConfigItemBase *m_last;
  
public:
  PersistentConfig(): m_last(0) {}

  inline bool CheckOverlaps() const;
};


class PersistentConfigItemBase {
protected:
  size_t m_address;
  size_t m_size;
  PersistentConfigItemBase *m_next;
  
  PersistentConfigItemBase();
  PersistentConfigItemBase(const PersistentConfigItemBase &);
  
public:
  PersistentConfigItemBase(PersistentConfig &config, size_t address, size_t size): m_address(address), m_size(size), m_next(0)
  {
    // register self at the end of the items list
    m_next = config.m_last;
	config.m_last = this;
  }

  // recursive check of the overlaps of this item and all behind it
  bool CheckOverlaps() const
  {
    bool result = true;
	
    // if there are some more items
	if (m_next) {
	
	  // check recursively overlaps of this item with next and everything behind it
	  result = m_next->CheckOverlaps(*this);
	    
	  // check recursively the overlaps in the next item
	  result = result && m_next->CheckOverlaps();
	}
	
	return result;
  }
  
  // recursive check of the overlaps of this item and all behind it against the given item
  bool CheckOverlaps(const PersistentConfigItemBase &that) const
  {	
    // if this and that overlap
	if ((that.m_address <= m_address && m_address < (that.m_address + that.m_size)) || (m_address <= that.m_address && that.m_address < (m_address + m_size))) {
	  
	  // check fails
	  return false;
	}
	
	// if there are some more items
	if (m_next) {
	
	  // compare given that to the next item in the chain
	  return m_next->CheckOverlaps(that);
	}
	
	// no overlaps found
	return true;
  }
  
   void Read(void *data)
   {
     for (size_t i = 0; i < m_size; ++i)
	 {
	   ((byte *) data)[i] = EEPROM.read(m_address + i);
	 }
   }
   
   void Write(const void *data)
   {
     for (size_t i = 0; i < m_size; ++i)
	 {
	   EEPROM.write(m_address + i, ((const byte *) data)[i]);
	 }
   }
   
};

template <class T>
class PersistentConfigItem: public PersistentConfigItemBase {
private:
  
public:
  PersistentConfigItem(PersistentConfig &config, size_t address): PersistentConfigItemBase(config, address, sizeof(T)) {}
  
  operator T () 
  {
    T result;
	Read(&result);
	return result;
  }
  
  T &operator=(const T &that)
  {
    Write(&that);
  }
};

inline bool PersistentConfig::CheckOverlaps() const
{
  bool result = false;

  if (m_last) {
    result = m_last->CheckOverlaps();
  }

  return result;
}

#endif // PERSISTENT_CONFIG__H
