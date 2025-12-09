#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

const uint64_t MOD = 4294967296;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32(zero_point + n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const auto converted_zero_point = static_cast<uint64_t> (zero_point.raw_value_);
  const auto converted_this_value = static_cast<uint64_t> (this->raw_value_);
  return checkpoint + ((converted_this_value - converted_zero_point + MOD) % MOD);
}