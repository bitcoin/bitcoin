#include "uint256_t.build"
#include <vector>
#include <cstring>

const uint128_t uint128_64(64);
const uint128_t uint128_128(128);
const uint128_t uint128_256(256);
const uint256_t uint256_0(0);
const uint256_t uint256_1(1);
const uint256_t uint256_max(uint128_t((uint64_t) -1, (uint64_t) -1), uint128_t((uint64_t) -1, (uint64_t) -1));

uint256_t::uint256_t(const std::string & s) {
    init(s.c_str());
}

uint256_t::uint256_t(const char * s) {
    init(s);
}

uint256_t::uint256_t(const std::string & s, uint8_t base) {
    init_from_base(s.c_str(), base);
}

uint256_t::uint256_t(const char * s, uint8_t base) {
    init_from_base(s, base);
}

uint256_t::uint256_t(const bool & b)
    : uint256_t((uint8_t) b)
{}

void uint256_t::init(const char * s) {
    //create from string
    char buffer[64];
    if (s == NULL) { uint256_t(); return; }
    if (s[1] == 'x')
        s += 2;
    else if (*s == 'x')
        s++;

    int len = strlen(s);
    int padLength = 0;
    if (len < 64) {
        padLength = 64 - len;
        memset(buffer, '0', padLength);
    }

    memcpy(buffer + padLength, s, len);

    UPPER = uint128_t(buffer);
    LOWER = uint128_t(buffer + 32);
}

void uint256_t::init_from_base(const char * s, uint8_t base) {
    *this = 0;

    uint256_t power(1);
    uint8_t digit;
    int pos = strlen(s) - 1;
    while(pos >= 0) {
        digit = 0;
        if('0' <= s[pos] && s[pos] <= '9') {
            digit = s[pos] - '0';
        } else if('a' <= s[pos] && s[pos] <= 'z') {
            digit = s[pos] - 'a' + 10;
        }
        *this += digit * power;
        pos--;
        power *= base;
    }
}

uint256_t & uint256_t::operator=(const bool & rhs) {
    UPPER = 0;
    LOWER = rhs;
    return *this;
}

uint256_t::operator bool() const{
    return (bool) (UPPER | LOWER);
}

uint256_t::operator uint8_t() const{
    return (uint8_t) LOWER;
}

uint256_t::operator uint16_t() const{
    return (uint16_t) LOWER;
}

uint256_t::operator uint32_t() const{
    return (uint32_t) LOWER;
}

uint256_t::operator uint64_t() const{
    return (uint64_t) LOWER;
}

uint256_t::operator uint128_t() const{
    return LOWER;
}

uint256_t uint256_t::operator&(const uint128_t & rhs) const{
    return uint256_t(uint128_0, LOWER & rhs);
}

uint256_t uint256_t::operator&(const uint256_t & rhs) const{
    return uint256_t(UPPER & rhs.UPPER, LOWER & rhs.LOWER);
}

uint256_t & uint256_t::operator&=(const uint128_t & rhs){
    UPPER  = uint128_0;
    LOWER &= rhs;
    return *this;
}

uint256_t & uint256_t::operator&=(const uint256_t & rhs){
    UPPER &= rhs.UPPER;
    LOWER &= rhs.LOWER;
    return *this;
}

uint256_t uint256_t::operator|(const uint128_t & rhs) const{
    return uint256_t(UPPER , LOWER | rhs);
}

uint256_t uint256_t::operator|(const uint256_t & rhs) const{
    return uint256_t(UPPER | rhs.UPPER, LOWER | rhs.LOWER);
}

uint256_t & uint256_t::operator|=(const uint128_t & rhs){
    LOWER |= rhs;
    return *this;
}

uint256_t & uint256_t::operator|=(const uint256_t & rhs){
    UPPER |= rhs.UPPER;
    LOWER |= rhs.LOWER;
    return *this;
}

uint256_t uint256_t::operator^(const uint128_t & rhs) const{
    return uint256_t(UPPER, LOWER ^ rhs);
}

uint256_t uint256_t::operator^(const uint256_t & rhs) const{
    return uint256_t(UPPER ^ rhs.UPPER, LOWER ^ rhs.LOWER);
}

uint256_t & uint256_t::operator^=(const uint128_t & rhs){
    LOWER ^= rhs;
    return *this;
}

uint256_t & uint256_t::operator^=(const uint256_t & rhs){
    UPPER ^= rhs.UPPER;
    LOWER ^= rhs.LOWER;
    return *this;
}

uint256_t uint256_t::operator~() const{
    return uint256_t(~UPPER, ~LOWER);
}

uint256_t uint256_t::operator<<(const uint128_t & rhs) const{
    return *this << uint256_t(rhs);
}

uint256_t uint256_t::operator<<(const uint256_t & rhs) const{
    const uint128_t shift = rhs.LOWER;
    if (((bool) rhs.UPPER) || (shift >= uint128_256)){
        return uint256_0;
    }
    else if (shift == uint128_128){
        return uint256_t(LOWER, uint128_0);
    }
    else if (shift == uint128_0){
        return *this;
    }
    else if (shift < uint128_128){
        return uint256_t((UPPER << shift) + (LOWER >> (uint128_128 - shift)), LOWER << shift);
    }
    else if ((uint128_256 > shift) && (shift > uint128_128)){
        return uint256_t(LOWER << (shift - uint128_128), uint128_0);
    }
    else{
        return uint256_0;
    }
}

uint256_t & uint256_t::operator<<=(const uint128_t & shift){
    return *this <<= uint256_t(shift);
}

uint256_t & uint256_t::operator<<=(const uint256_t & shift){
    *this = *this << shift;
    return *this;
}

uint256_t uint256_t::operator>>(const uint128_t & rhs) const{
    return *this >> uint256_t(rhs);
}

uint256_t uint256_t::operator>>(const uint256_t & rhs) const{
    const uint128_t shift = rhs.LOWER;
    if (((bool) rhs.UPPER) | (shift >= uint128_256)){
        return uint256_0;
    }
    else if (shift == uint128_128){
        return uint256_t(UPPER);
    }
    else if (shift == uint128_0){
        return *this;
    }
    else if (shift < uint128_128){
        return uint256_t(UPPER >> shift, (UPPER << (uint128_128 - shift)) + (LOWER >> shift));
    }
    else if ((uint128_256 > shift) && (shift > uint128_128)){
        return uint256_t(UPPER >> (shift - uint128_128));
    }
    else{
        return uint256_0;
    }
}

uint256_t & uint256_t::operator>>=(const uint128_t & shift){
    return *this >>= uint256_t(shift);
}

uint256_t & uint256_t::operator>>=(const uint256_t & shift){
    *this = *this >> shift;
    return *this;
}

bool uint256_t::operator!() const{
    return ! (bool) *this;
}

bool uint256_t::operator&&(const uint128_t & rhs) const{
    return (*this && uint256_t(rhs));
}

bool uint256_t::operator&&(const uint256_t & rhs) const{
    return ((bool) *this && (bool) rhs);
}

bool uint256_t::operator||(const uint128_t & rhs) const{
    return (*this || uint256_t(rhs));
}

bool uint256_t::operator||(const uint256_t & rhs) const{
    return ((bool) *this || (bool) rhs);
}

bool uint256_t::operator==(const uint128_t & rhs) const{
    return (*this == uint256_t(rhs));
}

bool uint256_t::operator==(const uint256_t & rhs) const{
    return ((UPPER == rhs.UPPER) && (LOWER == rhs.LOWER));
}

bool uint256_t::operator!=(const uint128_t & rhs) const{
    return (*this != uint256_t(rhs));
}

bool uint256_t::operator!=(const uint256_t & rhs) const{
    return ((UPPER != rhs.UPPER) | (LOWER != rhs.LOWER));
}

bool uint256_t::operator>(const uint128_t & rhs) const{
    return (*this > uint256_t(rhs));
}

bool uint256_t::operator>(const uint256_t & rhs) const{
    if (UPPER == rhs.UPPER){
        return (LOWER > rhs.LOWER);
    }
    if (UPPER > rhs.UPPER){
        return true;
    }
    return false;
}

bool uint256_t::operator<(const uint128_t & rhs) const{
    return (*this < uint256_t(rhs));
}

bool uint256_t::operator<(const uint256_t & rhs) const{
    if (UPPER == rhs.UPPER){
        return (LOWER < rhs.LOWER);
    }
    if (UPPER < rhs.UPPER){
        return true;
    }
    return false;
}

bool uint256_t::operator>=(const uint128_t & rhs) const{
    return (*this >= uint256_t(rhs));
}

bool uint256_t::operator>=(const uint256_t & rhs) const{
    return ((*this > rhs) | (*this == rhs));
}

bool uint256_t::operator<=(const uint128_t & rhs) const{
    return (*this <= uint256_t(rhs));
}

bool uint256_t::operator<=(const uint256_t & rhs) const{
    return ((*this < rhs) | (*this == rhs));
}

uint256_t uint256_t::operator+(const uint128_t & rhs) const{
    return *this + uint256_t(rhs);
}

uint256_t uint256_t::operator+(const uint256_t & rhs) const{
    return uint256_t(UPPER + rhs.UPPER + (((LOWER + rhs.LOWER) < LOWER)?uint128_1:uint128_0), LOWER + rhs.LOWER);
}

uint256_t & uint256_t::operator+=(const uint128_t & rhs){
    return *this += uint256_t(rhs);
}

uint256_t & uint256_t::operator+=(const uint256_t & rhs){
    UPPER = rhs.UPPER + UPPER + ((LOWER + rhs.LOWER) < LOWER);
    LOWER = LOWER + rhs.LOWER;
    return *this;
}

uint256_t uint256_t::operator-(const uint128_t & rhs) const{
    return *this - uint256_t(rhs);
}

uint256_t uint256_t::operator-(const uint256_t & rhs) const{
    return uint256_t(UPPER - rhs.UPPER - ((LOWER - rhs.LOWER) > LOWER), LOWER - rhs.LOWER);
}

uint256_t & uint256_t::operator-=(const uint128_t & rhs){
    return *this -= uint256_t(rhs);
}

uint256_t & uint256_t::operator-=(const uint256_t & rhs){
    *this = *this - rhs;
    return *this;
}

uint256_t uint256_t::operator*(const uint128_t & rhs) const{
    return *this * uint256_t(rhs);
}

uint256_t uint256_t::operator*(const uint256_t & rhs) const{
    // split values into 4 64-bit parts
    uint128_t top[4] = {UPPER.upper(), UPPER.lower(), LOWER.upper(), LOWER.lower()};
    uint128_t bottom[4] = {rhs.upper().upper(), rhs.upper().lower(), rhs.lower().upper(), rhs.lower().lower()};
    uint128_t products[4][4];

    // multiply each component of the values
    for(int y = 3; y > -1; y--){
        for(int x = 3; x > -1; x--){
            products[3 - y][x] = top[x] * bottom[y];
        }
    }

    // first row
    uint128_t fourth64 = uint128_t(products[0][3].lower());
    uint128_t third64  = uint128_t(products[0][2].lower()) + uint128_t(products[0][3].upper());
    uint128_t second64 = uint128_t(products[0][1].lower()) + uint128_t(products[0][2].upper());
    uint128_t first64  = uint128_t(products[0][0].lower()) + uint128_t(products[0][1].upper());

    // second row
    third64  += uint128_t(products[1][3].lower());
    second64 += uint128_t(products[1][2].lower()) + uint128_t(products[1][3].upper());
    first64  += uint128_t(products[1][1].lower()) + uint128_t(products[1][2].upper());

    // third row
    second64 += uint128_t(products[2][3].lower());
    first64  += uint128_t(products[2][2].lower()) + uint128_t(products[2][3].upper());

    // fourth row
    first64  += uint128_t(products[3][3].lower());

    // combines the values, taking care of carry over
    return uint256_t(first64 << uint128_64, uint128_0) +
           uint256_t(third64.upper(), third64 << uint128_64) +
           uint256_t(second64, uint128_0) +
           uint256_t(fourth64);
}

uint256_t & uint256_t::operator*=(const uint128_t & rhs){
    return *this *= uint256_t(rhs);
}

uint256_t & uint256_t::operator*=(const uint256_t & rhs){
    *this = *this * rhs;
    return *this;
}

std::pair <uint256_t, uint256_t> uint256_t::divmod(const uint256_t & lhs, const uint256_t & rhs) const{
    // Save some calculations /////////////////////
    if (rhs == uint256_0){
        throw std::domain_error("Error: division or modulus by 0");
    }
    else if (rhs == uint256_1){
        return std::pair <uint256_t, uint256_t> (lhs, uint256_0);
    }
    else if (lhs == rhs){
        return std::pair <uint256_t, uint256_t> (uint256_1, uint256_0);
    }
    else if ((lhs == uint256_0) || (lhs < rhs)){
        return std::pair <uint256_t, uint256_t> (uint256_0, lhs);
    }

    std::pair <uint256_t, uint256_t> qr(uint256_0, lhs);
    uint256_t copyd = rhs << (lhs.bits() - rhs.bits());
    uint256_t adder = uint256_1 << (lhs.bits() - rhs.bits());
    if (copyd > qr.second){
        copyd >>= uint256_1;
        adder >>= uint256_1;
    }
    while (qr.second >= rhs){
        if (qr.second >= copyd){
            qr.second -= copyd;
            qr.first |= adder;
        }
        copyd >>= uint256_1;
        adder >>= uint256_1;
    }
    return qr;
}

uint256_t uint256_t::operator/(const uint128_t & rhs) const{
    return *this / uint256_t(rhs);
}

uint256_t uint256_t::operator/(const uint256_t & rhs) const{
    return divmod(*this, rhs).first;
}

uint256_t & uint256_t::operator/=(const uint128_t & rhs){
    return *this /= uint256_t(rhs);
}

uint256_t & uint256_t::operator/=(const uint256_t & rhs){
    *this = *this / rhs;
    return *this;
}

uint256_t uint256_t::operator%(const uint128_t & rhs) const{
    return *this % uint256_t(rhs);
}

uint256_t uint256_t::operator%(const uint256_t & rhs) const{
    return *this - (rhs * (*this / rhs));
}

uint256_t & uint256_t::operator%=(const uint128_t & rhs){
    return *this %= uint256_t(rhs);
}

uint256_t & uint256_t::operator%=(const uint256_t & rhs){
    *this = *this % rhs;
    return *this;
}

uint256_t & uint256_t::operator++(){
    *this += uint256_1;
    return *this;
}

uint256_t uint256_t::operator++(int){
    uint256_t temp(*this);
    ++*this;
    return temp;
}

uint256_t & uint256_t::operator--(){
    *this -= uint256_1;
    return *this;
}

uint256_t uint256_t::operator--(int){
    uint256_t temp(*this);
    --*this;
    return temp;
}

uint256_t uint256_t::operator+() const{
    return *this;
}

uint256_t uint256_t::operator-() const{
    return ~*this + uint256_1;
}

const uint128_t & uint256_t::upper() const {
    return UPPER;
}

const uint128_t & uint256_t::lower() const {
    return LOWER;
}

std::vector<uint8_t> uint256_t::export_bits() const {
    std::vector<uint8_t> ret;
    ret.reserve(32);
    UPPER.export_bits(ret);
    LOWER.export_bits(ret);
    return ret;
}

std::vector<uint8_t> uint256_t::export_bits_truncate() const {
    std::vector<uint8_t> ret = export_bits();

	//prune the zeroes
	int i = 0;
	while (ret[i] == 0 && i < 64) i++;
	ret.erase(ret.begin(), ret.begin() + i);

	return ret;
}

uint16_t uint256_t::bits() const{
    uint16_t out = 0;
    if (UPPER){
        out = 128;
        uint128_t up = UPPER;
        while (up){
            up >>= uint128_1;
            out++;
        }
    }
    else{
        uint128_t low = LOWER;
        while (low){
            low >>= uint128_1;
            out++;
        }
    }
    return out;
}

std::string uint256_t::str(uint8_t base, const unsigned int & len) const{
    if ((base < 2) || (base > 36)){
        throw std::invalid_argument("Base must be in the range 2-36");
    }
    std::string out = "";
    if (!(*this)){
        out = "0";
    }
    else{
        std::pair <uint256_t, uint256_t> qr(*this, uint256_0);
        do{
            qr = divmod(qr.first, base);
            out = "0123456789abcdefghijklmnopqrstuvwxyz"[(uint8_t) qr.second] + out;
        } while (qr.first);
    }
    if (out.size() < len){
        out = std::string(len - out.size(), '0') + out;
    }
    return out;
}

uint256_t operator&(const uint128_t & lhs, const uint256_t & rhs){
    return rhs & lhs;
}

uint128_t & operator&=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (rhs & lhs).lower();
    return lhs;
}

uint256_t operator|(const uint128_t & lhs, const uint256_t & rhs){
    return rhs | lhs;
}

uint128_t & operator|=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (rhs | lhs).lower();
    return lhs;
}

uint256_t operator^(const uint128_t & lhs, const uint256_t & rhs){
    return rhs ^ lhs;
}

uint128_t & operator^=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (rhs ^ lhs).lower();
    return lhs;
}

uint256_t operator<<(const bool & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const uint8_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const uint16_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const uint32_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const uint64_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const uint128_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const int8_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const int16_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const int32_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint256_t operator<<(const int64_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) << rhs;
}

uint128_t & operator<<=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (uint256_t(lhs) << rhs).lower();
    return lhs;
}

uint256_t operator>>(const bool & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const uint8_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const uint16_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const uint32_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const uint64_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const uint128_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const int8_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const int16_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const int32_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint256_t operator>>(const int64_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) >> rhs;
}

uint128_t & operator>>=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (uint256_t(lhs) >> rhs).lower();
    return lhs;
}

// Comparison Operators
bool operator==(const uint128_t & lhs, const uint256_t & rhs){
    return rhs == lhs;
}

bool operator!=(const uint128_t & lhs, const uint256_t & rhs){
    return rhs != lhs;
}

bool operator>(const uint128_t & lhs, const uint256_t & rhs){
    return rhs < lhs;
}

bool operator<(const uint128_t & lhs, const uint256_t & rhs){
    return rhs > lhs;
}

bool operator>=(const uint128_t & lhs, const uint256_t & rhs){
    return rhs <= lhs;
}

bool operator<=(const uint128_t & lhs, const uint256_t & rhs){
    return rhs >= lhs;
}

// Arithmetic Operators
uint256_t operator+(const uint128_t & lhs, const uint256_t & rhs){
    return rhs + lhs;
}

uint128_t & operator+=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (rhs + lhs).lower();
    return lhs;
}

uint256_t operator-(const uint128_t & lhs, const uint256_t & rhs){
    return -(rhs - lhs);
}

uint128_t & operator-=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (-(rhs - lhs)).lower();
    return lhs;
}

uint256_t operator*(const uint128_t & lhs, const uint256_t & rhs){
    return rhs * lhs;
}

uint128_t & operator*=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (rhs * lhs).lower();
    return lhs;
}

uint256_t operator/(const uint128_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) / rhs;
}

uint128_t & operator/=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (uint256_t(lhs) / rhs).lower();
    return lhs;
}

uint256_t operator%(const uint128_t & lhs, const uint256_t & rhs){
    return uint256_t(lhs) % rhs;
}

uint128_t & operator%=(uint128_t & lhs, const uint256_t & rhs){
    lhs = (uint256_t(lhs) % rhs).lower();
    return lhs;
}

std::ostream & operator<<(std::ostream & stream, const uint256_t & rhs){
    if (stream.flags() & stream.oct){
        stream << rhs.str(8);
    }
    else if (stream.flags() & stream.dec){
        stream << rhs.str(10);
    }
    else if (stream.flags() & stream.hex){
        stream << rhs.str(16);
    }
    return stream;
}
