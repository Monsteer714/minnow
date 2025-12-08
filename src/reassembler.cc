#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto& writer = getWriter();
  if ( data.empty() && is_last_substring && first_index == nextIndex() ) {
    writer.close();
    return;
  }

  if ( is_last_substring ) {
    last_byte = first_index + data.length() - 1;
  }

  if ( first_index < nextIndex() + availableCapacity() && first_index + data.length() > nextIndex() ) {
    const uint64_t insert_index = std::max( first_index, nextIndex() );

    map.insert( { insert_index,
                  data.substr( insert_index - first_index,
                               std::min( static_cast<uint64_t>( data.length() ),
                                         nextIndex() + availableCapacity() - insert_index ) ) } );
    mergeStrings();
    auto it = map.begin();
    if ( it->first == nextIndex() ) {
      writer.push( it->second );
      if ( it->first + it->second.length() - 1 == last_byte ) {
        writer.close();
      }
      map.erase( it );
    }
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t res = 0;
  for ( const auto& [k, v] : map ) {
    res += v.length();
  }
  return res;
}

void Reassembler::mergeStrings()
{
  if ( map.size() <= 1 ) {
    return;
  }

  auto it = map.begin();
  auto next = std::next( it );
  while ( next != map.end() ) {
    if ( it->first + it->second.length() >= next->first ) {
      if ( next->first + next->second.length() <= it->first + it->second.length() ) {
        next = map.erase( next );
        continue;
      }

      const uint64_t insert_index = it->first + it->second.length() - next->first;
      it->second += next->second.substr( insert_index );
      next = map.erase( ( next ) );
    } else {
      it++;
      next++;
    }
  }
}