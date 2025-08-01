/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#pragma once
#include "qtype.hh"
#include "dnsname.hh"
#include <ctime>
#include <optional>
#include <string_view>
#include <sys/types.h>

#undef BADSIG  // signal.h SIG_ERR

struct DNSRecord;

class RCode
{
public:
  enum rcodes_ : uint8_t { NoError=0, FormErr=1, ServFail=2, NXDomain=3, NotImp=4, Refused=5, YXDomain=6, YXRRSet=7, NXRRSet=8, NotAuth=9, NotZone=10};
  static std::string to_s(uint8_t rcode);
  static std::string to_short_s(uint8_t rcode);
  static std::optional<uint8_t> from_short(const std::string_view& rcode_string);
  const static std::array<std::string, 24> rcodes_s;
};

class ERCode
{
public:
  enum rcodes_ : uint16_t { BADVERS=16, BADSIG=16, BADKEY=17, BADTIME=18, BADMODE=19, BADNAME=20, BADALG=21, BADTRUNC=22, BADCOOKIE=23 };
  static std::string to_s(uint16_t rcode);
  static std::string to_short_s(uint16_t rcode);
  static std::optional<uint16_t> from_short(const std::string_view& ercode_string);
};

class Opcode
{
public:
  enum opcodes_ : uint8_t { Query=0, IQuery=1, Status=2, Notify=4, Update=5 };
  static std::string to_s(uint8_t opcode);
  static std::optional<uint8_t> from_lowercase_string(const std::string_view& opcode_string);
};

// This needs to be a signed type, so that serialization of UnknownDomainID
// as text is "-1", for compatibility with the remote and pipe backends.
// See static_assert there for details.
using domainid_t = int32_t;

constexpr domainid_t UnknownDomainID{-1};

//! This class represents a resource record
class DNSResourceRecord
{
public:
  static DNSResourceRecord fromWire(const DNSRecord& wire);

  enum Place : uint8_t
  {
    QUESTION = 0,
    ANSWER = 1,
    AUTHORITY = 2,
    ADDITIONAL = 3
  }; //!< Type describing the positioning within, say, a DNSPacket

  [[nodiscard]] static std::string placeString(uint8_t place);
  void setContent(const string& content);
  [[nodiscard]] string getZoneRepresentation(bool noDot = false) const;

  // data
  DNSName qname; //!< the name of this record, for example: www.powerdns.com
  DNSName ordername;
  DNSName wildcardname;
  string content; //!< what this record points to. Example: 10.1.2.3

  // Aligned on 8-byte boundaries on systems where time_t is 8 bytes and int
  // is 4 bytes, aka modern linux on x86_64
  time_t last_modified{}; //!< Timestamp of last update, if known by the backend

  uint32_t ttl{}; //!< Time To Live of this record
  uint32_t signttl{}; //!< If non-zero, use this TTL as original TTL in the RRSIG

  domainid_t domain_id{UnknownDomainID}; //!< If a backend implements this, the domain_id of the zone this record is in
  QType qtype; //!< qtype of this record, ie A, CNAME, MX etc
  uint16_t qclass{1}; //!< class of this record

  uint8_t scopeMask{};
  bool auth{true};
  bool disabled{};

  bool operator==(const DNSResourceRecord& rhs);

  bool operator<(const DNSResourceRecord& other) const
  {
    if (qname < other.qname) {
      return true;
    }
    if (qname == other.qname) {
      return (content < other.content);
    }
    return false;
  }
};

#define GCCPACKATTRIBUTE __attribute__((packed))

struct dnsrecordheader
{
  uint16_t d_type;
  uint16_t d_class;
  uint32_t d_ttl;
  uint16_t d_clen;
} GCCPACKATTRIBUTE;

struct EDNS0Record
{
  uint8_t extRCode, version;
  uint16_t extFlags;
} GCCPACKATTRIBUTE;

static_assert(sizeof(EDNS0Record) == 4, "EDNS0Record size must be 4");

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
#include <machine/endian.h>
#elif __linux__ || __GNU__
# include <endian.h>

#else  // with thanks to <arpa/nameser.h>

# define LITTLE_ENDIAN   1234    /* least-significant byte first (vax, pc) */
# define BIG_ENDIAN      4321    /* most-significant byte first (IBM, net) */
# define PDP_ENDIAN      3412    /* LSB first in word, MSW first in long (pdp) */

#if defined(vax) || defined(ns32000) || defined(sun386) || defined(i386) || \
        defined(__i386) || defined(__ia64) || defined(__amd64) || \
        defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) || \
        defined(__alpha__) || defined(__alpha) || \
        (defined(__Lynx__) && defined(__x86__))
# define BYTE_ORDER      LITTLE_ENDIAN
#endif

#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
    defined(__sparc) || \
    defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
    defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) ||\
    defined(apollo) || defined(__convex__) || defined(_CRAY) || \
    defined(__hppa) || defined(__hp9000) || \
    defined(__hp9000s300) || defined(__hp9000s700) || \
    defined(__hp3000s900) || defined(MPE) || \
    defined(BIT_ZERO_ON_LEFT) || defined(m68k) || \
        (defined(__Lynx__) && \
        (defined(__68k__) || defined(__sparc__) || defined(__powerpc__)))
# define BYTE_ORDER      BIG_ENDIAN
#endif

#endif

struct dnsheader {
        uint16_t        id;             /* query identification number */
#if BYTE_ORDER == BIG_ENDIAN
                        /* fields in third byte */
        unsigned        qr: 1;          /* response flag */
        unsigned        opcode: 4;      /* purpose of message */
        unsigned        aa: 1;          /* authoritative answer */
        unsigned        tc: 1;          /* truncated message */
        unsigned        rd: 1;          /* recursion desired */
                        /* fields in fourth byte */
        unsigned        ra: 1;          /* recursion available */
        unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        unsigned        ad: 1;          /* authentic data from named */
        unsigned        cd: 1;          /* checking disabled by resolver */
        unsigned        rcode :4;       /* response code */
#elif BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
                        /* fields in third byte */
        unsigned        rd :1;          /* recursion desired */
        unsigned        tc :1;          /* truncated message */
        unsigned        aa :1;          /* authoritative answer */
        unsigned        opcode :4;      /* purpose of message */
        unsigned        qr :1;          /* response flag */
                        /* fields in fourth byte */
        unsigned        rcode :4;       /* response code */
        unsigned        cd: 1;          /* checking disabled by resolver */
        unsigned        ad: 1;          /* authentic data from named */
        unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        unsigned        ra :1;          /* recursion available */
#endif
                        /* remaining bytes */
        uint16_t        qdcount;        /* number of question entries */
        uint16_t        ancount;        /* number of answer entries */
        uint16_t        nscount;        /* number of authority entries */
        uint16_t        arcount;        /* number of resource entries */
};

static_assert(sizeof(dnsheader) == 12, "dnsheader size must be 12");

class dnsheader_aligned
{
public:
  static bool isMemoryAligned(const void* mem)
  {
    return reinterpret_cast<uintptr_t>(mem) % sizeof(uint32_t) == 0; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  }

  dnsheader_aligned(const void* mem)
  {
    if (isMemoryAligned(mem)) {
      d_p = reinterpret_cast<const dnsheader*>(mem);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }
    else {
      memcpy(&d_h, mem, sizeof(dnsheader));
      d_p = &d_h;
    }
  }

  [[nodiscard]] const dnsheader* get() const
  {
    return d_p;
  }

  [[nodiscard]] const dnsheader& operator*() const
  {
    return *d_p;
  }

  [[nodiscard]] const dnsheader* operator->() const
  {
    return d_p;
  }

private:
  dnsheader d_h{};
  const dnsheader* d_p{};
};

inline uint16_t* getFlagsFromDNSHeader(dnsheader* dh)
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<uint16_t*>(reinterpret_cast<char*>(dh) + sizeof(uint16_t));
}

inline const uint16_t * getFlagsFromDNSHeader(const dnsheader* dh)
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const uint16_t*>(reinterpret_cast<const char*>(dh) + sizeof(uint16_t));
}

#define DNS_TYPE_SIZE (2)
#define DNS_CLASS_SIZE (2)
#define DNS_TTL_SIZE (4)
#define DNS_RDLENGTH_SIZE (2)
#define EDNS_EXTENDED_RCODE_SIZE (1)
#define EDNS_VERSION_SIZE (1)
#define EDNS_OPTION_CODE_SIZE (2)
#define EDNS_OPTION_LENGTH_SIZE (2)

#if BYTE_ORDER == BIG_ENDIAN
#define FLAGS_RD_OFFSET (8)
#define FLAGS_CD_OFFSET (12)
#elif BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
#define FLAGS_RD_OFFSET (0)
#define FLAGS_CD_OFFSET (12)
#endif

uint32_t hashQuestion(const uint8_t* packet, uint16_t len, uint32_t init, bool& ok);

struct TSIGTriplet
{
  DNSName name, algo;
  string secret;
};
