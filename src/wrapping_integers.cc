#include "wrapping_integers.hh"
#include <cstdlib>

using namespace std;

const uint64_t MOD = 4294967296;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32( zero_point + n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const auto converted_zero_point = static_cast<uint64_t>( zero_point.raw_value_ );
  const auto converted_this_value = static_cast<uint64_t>( this->raw_value_ );

  const uint64_t offset = (converted_this_value - converted_zero_point + MOD) % MOD;
  const uint64_t mid = (( checkpoint / MOD ) * MOD) + offset;
  const uint64_t down = (mid >= MOD) ? mid - MOD : UINT64_MAX;
  const uint64_t up = mid + MOD;

  auto abs_diff = [&](uint64_t a, uint64_t b) -> uint64_t {
    return (a > b) ? (a - b) : (b - a);
  };

  auto diff1 = abs_diff(checkpoint, down);
  auto diff2 = abs_diff(checkpoint, mid);
  auto diff3 = abs_diff(checkpoint, up);
  uint64_t res = 0;
  if (diff1 <= diff2 && diff1 <= diff3) {
    res = down;
  } else if (diff2 <= diff1 && diff2 <= diff3) {
    res = mid;
  } else {
    res = up;
  }
  return res;
}