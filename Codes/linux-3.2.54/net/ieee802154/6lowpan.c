/*
 * Copyright 2011, Siemens AG
 * written by Alexander Smirnov <alex.bluesman.smirnov@gmail.com>
 */

/*
 * Based on patches from Jon Smirl <jonsmirl@gmail.com>
 * Copyright (c) 2011 Jon Smirl <jonsmirl@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Jon's code is based on 6lowpan implementation for Contiki which is:
 * Copyright (c) 2008, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define DEBUG

#include <linux/bitops.h>
#include <linux/if_arp.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <net/af_ieee802154.h>
#include <net/ieee802154.h>
#include <net/ieee802154_netdev.h>
#include <net/ipv6.h>

#include "6lowpan.h"

/* TTL uncompression values */
static const u8 lowpan_ttl_values[] = {0, 1, 64, 255};

static LIST_HEAD(lowpan_devices);

/*
 * Uncompression of linklocal:
 *   0 -> 16 bytes from packet
 *   1 -> 2  bytes from prefix - bunch of zeroes and 8 from packet
 *   2 -> 2  bytes from prefix - zeroes + 2 from packet
 *   3 -> 2  bytes from prefix - infer 8 bytes from lladdr
 *
 *  NOTE: => the uncompress function does change 0xf to 0x10
 *  NOTE: 0x00 => no-autoconfig => unspecified
 */
static const u8 lowpan_unc_llconf[] = {0x0f, 0x28, 0x22, 0x20};

/*
 * Uncompression of ctx-based:
 *   0 -> 0 bits  from packet [unspecified / reserved]
 *   1 -> 8 bytes from prefix - bunch of zeroes and 8 from packet
 *   2 -> 8 bytes from prefix - zeroes + 2 from packet
 *   3 -> 8 bytes from prefix - infer 8 bytes from lladdr
 */
static const u8 lowpan_unc_ctxconf[] = {0x00, 0x88, 0x82, 0x80};

/*
 * Uncompression of ctx-base
 *   0 -> 0 bits from packet
 *   1 -> 2 bytes from prefix - bunch of zeroes 5 from packet
 *   2 -> 2 bytes from prefix - zeroes + 3 from packet
 *   3 -> 2 bytes from prefix - infer 1 bytes from lladdr
 */
static const u8 lowpan_unc_mxconf[] = {0x0f, 0x25, 0x23, 0x21};

/* Link local prefix */
static const u8 lowpan_llprefix[] = {0xfe, 0x80};

/* private device info */
struct lowpan_dev_info {
	struct net_device	*real_dev; /* real WPAN device ptr */
	struct mutex		dev_list_mtx; /* mutex for list ops */
};

struct lowpan_dev_record {
	struct net_device *ldev;
	struct list_head list;
};

static inline struct
lowpan_dev_info *lowpan_dev_info(const struct net_device *dev)
{
	return netdev_priv(dev);
}

static inline void lowpan_address_flip(u8 *src, u8 *dest)
{
	int i;
	for (i = 0; i < IEEE802154_ADDR_LEN; i++)
		(dest)[IEEE802154_ADDR_LEN - i - 1] = (src)[i];
}

/* list of all 6lowpan devices, uses for package delivering */
/* print data in line */
static inline void lowpan_raw_dump_inline(const char *caller, char *msg,
				   unsigned char *buf, int len)
{
#ifdef DEBUG
	if (msg)
		pr_debug("(%s) %s: ", caller, msg);
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE,
		       16, 1, buf, len, false);
#endif /* DEBUG */
}

/*
 * print data in a table format:
 *
 * addr: xx xx xx xx xx xx
 * addr: xx xx xx xx xx xx
 * ...
 */
static inline void lowpan_raw_dump_table(const char *caller, char *msg,
				   unsigned char *buf, int len)
{
#ifdef DEBUG
	if (msg)
		pr_debug("(%s) %s:\n", caller, msg);
	print_hex_dump(KERN_DEBUG, "\t", DUMP_PREFIX_OFFSET,
		       16, 1, buf, len, false);
#endif /* DEBUG */
}

static u8
lowpan_compress_addr_64(u8 **hc06_ptr, u8 shift, const struct in6_addr *ipaddr,
		 const unsigned char *lladdr)
{
	u8 val = 0;

	if (is_addr_mac_addr_based(ipaddr, lladdr))
		val = 3; /* 0-bits */
	else if (lowpan_is_iid_16_bit_compressable(ipaddr)) {
		/* compress IID to 16 bits xxxx::XXXX */
		memcpy(*hc06_ptr, &ipaddr->s6_addr16[7], 2);
		*hc06_ptr += 2;
		val = 2; /* 16-bits */
	} else {
		/* do not compress IID => xxxx::IID */
		memcpy(*hc06_ptr, &ipaddr->s6_addr16[4], 8);
		*hc06_ptr += 8;
		val = 1; /* 64-bits */
	}

	return rol8(val, shift);
}

static void
lowpan_uip_ds6_set_addr_iid(struct in6_addr *ipaddr, unsigned char *lladdr)
{
	memcpy(&ipaddr->s6_addr[8], lladdr, IEEE802154_ALEN);
	/* second bit-flip (Universe/Local) is done according RFC2464 */
	ipaddr->s6_addr[8] ^= 0x02;
}

/*
 * Uncompress addresses based on a prefix and a postfix with zeroes in
 * between. If the postfix is zero in length it will use the link address
 * to configure the IP address (autoconf style).
 * pref_post_count takes a byte where the first nibble specify prefix count
 * and the second postfix count (NOTE: 15/0xf => 16 bytes copy).
 */
static int
lowpan_uncompress_addr(struct sk_buff *skb, struct in6_addr *ipaddr,
	u8 const *prefix, u8 pref_post_count, unsigned char *lladdr)
{
	u8 prefcount = pref_post_count >> 4;
	u8 postcount = pref_post_count & 0x0f;

	/* full nibble 15 => 16 */
	prefcount = (prefcount == 15 ? 16 : prefcount);
	postcount = (postcount == 15 ? 16 : postcount);

	if (lladdr)
		lowpan_raw_dump_inline(__func__, "linklocal address",
						lladdr,	IEEE802154_ALEN);
	if (prefcount > 0)
		memcpy(ipaddr, prefix, prefcount);

	if (prefcount + postcount < 16)
		memset(&ipaddr->s6_addr[prefcount], 0,
					16 - (prefcount + postcount));

	if (postcount > 0) {
		memcpy(&ipaddr->s6_addr[16 - postcount], skb->data, postcount);
		skb_pull(skb, postcount);
	} else if (prefcount > 0) {
		if (lladdr == NULL)
			return -EINVAL;

		/* no IID based configuration if no prefix and no data */
		lowpan_uip_ds6_set_addr_iid(ipaddr, lladdr);
	}

	pr_debug("(%s): uncompressing %d + %d => ", __func__, prefcount,
								postcount);
	lowpan_raw_dump_inline(NULL, NULL, ipaddr->s6_addr, 16);

	return 0;
}

static u8 lowpan_fetch_skb_u8(struct sk_buff *skb)
{
	u8 ret;

	ret = skb->data[0];
	skb_pull(skb, 1);

	return ret;
}

static int lowpan_header_create(struct sk_buff *skb,
			   struct net_device *dev,
			   unsigned short type, const void *_daddr,
			   const void *_saddr, unsigned len)
{
	u8 tmp, iphc0, iphc1, *hc06_ptr;
	struct ipv6hdr *hdr;
	const u8 *saddr = _saddr;
	const u8 *daddr = _daddr;
	u8 *head;
	struct ieee802154_addr sa, da;

	if (type != ETH_P_IPV6)
		return 0;
		/* TODO:
		 * if this package isn't ipv6 one, where should it be routed?
		 */
	head = kzalloc(100, GFP_KERNEL);
	if (head == NULL)
		return -ENOMEM;

	hdr = ipv6_hdr(skb);
	hc06_ptr = head + 2;

	pr_debug("(%s): IPv6 header dump:\n\tversion = %d\n\tlength  = %d\n"
		 "\tnexthdr = 0x%02x\n\thop_lim = %d\n", __func__,
		hdr->version, ntohs(hdr->payload_len), hdr->nexthdr,
		hdr->hop_limit);

	lowpan_raw_dump_table(__func__, "raw skb network header dump",
		skb_network_header(skb), sizeof(struct ipv6hdr));

	if (!saddr)
		saddr = dev->dev_addr;

	lowpan_raw_dump_inline(__func__, "saddr", (unsigned char *)saddr, 8);

	/*
	 * As we copy some bit-length fields, in the IPHC encoding bytes,
	 * we sometimes use |=
	 * If the field is 0, and the current bit value in memory is 1,
	 * this does not work. We therefore reset the IPHC encoding here
	 */
	iphc0 = LOWPAN_DISPATCH_IPHC;
	iphc1 = 0;

	/* TODO: context lookup */

	lowpan_raw_dump_inline(__func__, "daddr", (unsigned char *)daddr, 8);

	/*
	 * Traffic class, flow label
	 * If flow label is 0, compress it. If traffic class is 0, compress it
	 * We have to process both in the same time as the offset of traffic
	 * class depends on the presence of version and flow label
	 */

	/* hc06 format of TC is ECN | DSCP , original one is DSCP | ECN */
	tmp = (hdr->priority << 4) | (hdr->flow_lbl[0] >> 4);
	tmp = ((tmp & 0x03) << 6) | (tmp >> 2);

	if (((hdr->flow_lbl[0] & 0x0F) == 0) &&
	     (hdr->flow_lbl[1] == 0) && (hdr->flow_lbl[2] == 0)) {
		/* flow label can be compressed */
		iphc0 |= LOWPAN_IPHC_FL_C;
		if ((hdr->priority == 0) &&
		   ((hdr->flow_lbl[0] & 0xF0) == 0)) {
			/* compress (elide) all */
			iphc0 |= LOWPAN_IPHC_TC_C;
		} else {
			/* compress only the flow label */
			*hc06_ptr = tmp;
			hc06_ptr += 1;
		}
	} else {
		/* Flow label cannot be compressed */
		if ((hdr->priority == 0) &&
		   ((hdr->flow_lbl[0] & 0xF0) == 0)) {
			/* compress only traffic class */
			iphc0 |= LOWPAN_IPHC_TC_C;
			*hc06_ptr = (tmp & 0xc0) | (hdr->flow_lbl[0] & 0x0F);
			memcpy(hc06_ptr + 1, &hdr->flow_lbl[1], 2);
			hc06_ptr += 3;
		} else {
			/* compress nothing */
			memcpy(hc06_ptr, &hdr, 4);
			/* replace the top byte with new ECN | DSCP format */
			*hc06_ptr = tmp;
			hc06_ptr += 4;
		}
	}

	/* NOTE: payload length is always compressed */

	/* Next Header is compress if UDP */
	if (hdr->nexthdr == UIP_PROTO_UDP)
		iphc0 |= LOWPAN_IPHC_NH_C;

/* TODO: next header compression */

	if ((iphc0 & LOWPAN_IPHC_NH_C) == 0) {
		*hc06_ptr = hdr->nexthdr;
		hc06_ptr += 1;
	}

	/*
	 * Hop limit
	 * if 1:   compress, encoding is 01
	 * if 64:  compress, encoding is 10
	 * if 255: compress, encoding is 11
	 * else do not compress
	 */
	switch (hdr->hop_limit) {
	case 1:
		iphc0 |= LOWPAN_IPHC_TTL_1;
		break;
	case 64:
		iphc0 |= LOWPAN_IPHC_TTL_64;
		break;
	case 255:
		iphc0 |= LOWPAN_IPHC_TTL_255;
		break;
	default:
		*hc06_ptr = hdr->hop_limit;
		break;
	}

	/* source address compression */
	if (is_addr_unspecified(&hdr->saddr)) {
		pr_debug("(%s): source address is unspecified, setting SAC\n",
								__func__);
		iphc1 |= LOWPAN_IPHC_SAC;
	/* TODO: context lookup */
	} else if (is_addr_link_local(&hdr->saddr)) {
		pr_debug("(%s): source address is link-local\n", __func__);
		iphc1 |= lowpan_compress_addr_64(&hc06_ptr,
				LOWPAN_IPHC_SAM_BIT, &hdr->saddr, saddr);
	} else {
		pr_debug("(%s): send the full source address\n", __func__);
		memcpy(hc06_ptr, &hdr->saddr.s6_addr16[0], 16);
		hc06_ptr += 16;
	}

	/* destination address compression */
	if (is_addr_mcast(&hdr->daddr)) {
		pr_debug("(%s): destination address is multicast", __func__);
		iphc1 |= LOWPAN_IPHC_M;
		if (lowpan_is_mcast_addr_compressable8(&hdr->daddr)) {
			pr_debug("compressed to 1 octet\n");
			iphc1 |= LOWPAN_IPHC_DAM_11;
			/* use last byte */
			*hc06_ptr = hdr->daddr.s6_addr[15];
			hc06_ptr += 1;
		} else if (lowpan_is_mcast_addr_compressable32(&hdr->daddr)) {
			pr_debug("compressed to 4 octets\n");
			iphc1 |= LOWPAN_IPHC_DAM_10;
			/* second byte + the last three */
			*hc06_ptr = hdr->daddr.s6_addr[1];
			memcpy(hc06_ptr + 1, &hdr->daddr.s6_addr[13], 3);
			hc06_ptr += 4;
		} else if (lowpan_is_mcast_addr_compressable48(&hdr->daddr)) {
			pr_debug("compressed to 6 octets\n");
			iphc1 |= LOWPAN_IPHC_DAM_01;
			/* second byte + the last five */
			*hc06_ptr = hdr->daddr.s6_addr[1];
			memcpy(hc06_ptr + 1, &hdr->daddr.s6_addr[11], 5);
			hc06_ptr += 6;
		} else {
			pr_debug("using full address\n");
			iphc1 |= LOWPAN_IPHC_DAM_00;
			memcpy(hc06_ptr, &hdr->daddr.s6_addr[0], 16);
			hc06_ptr += 16;
		}
	} else {
		pr_debug("(%s): destination address is unicast: ", __func__);
		/* TODO: context lookup */
		if (is_addr_link_local(&hdr->daddr)) {
			pr_debug("destination address is link-local\n");
			iphc1 |= lowpan_compress_addr_64(&hc06_ptr,
				LOWPAN_IPHC_DAM_BIT, &hdr->daddr, daddr);
		} else {
			pr_debug("using full address\n");
			memcpy(hc06_ptr, &hdr->daddr.s6_addr16[0], 16);
			hc06_ptr += 16;
		}
	}

	/* TODO: UDP header compression */
	/* TODO: Next Header compression */

	head[0] = iphc0;
	head[1] = iphc1;

	skb_pull(skb, sizeof(struct ipv6hdr));
	memcpy(skb_push(skb, hc06_ptr - head), head, hc06_ptr - head);

	kfree(head);

	lowpan_raw_dump_table(__func__, "raw skb data dump", skb->data,
				skb->len);

	/*
	 * NOTE1: I'm still unsure about the fact that compression and WPAN
	 * header are created here and not later in the xmit. So wait for
	 * an opinion of net maintainers.
	 */
	/*
	 * NOTE2: to be absolutely correct, we must derive PANid information
	 * from MAC subif of the 'dev' and 'real_dev' network devices, but
	 * this isn't implemented in mainline yet, so currently we assign 0xff
	 */
	{
		/* prepare wpan address data */
		sa.addr_type = IEEE802154_ADDR_LONG;
		sa.pan_id = 0xff;

		da.addr_type = IEEE802154_ADDR_LONG;
		da.pan_id = 0xff;

		memcpy(&(da.hwaddr), daddr, 8);
		memcpy(&(sa.hwaddr), saddr, 8);

		mac_cb(skb)->flags = IEEE802154_FC_TYPE_DATA;
		return dev_hard_header(skb, lowpan_dev_info(dev)->real_dev,
				type, (void *)&da, (void *)&sa, skb->len);
	}
}

static int lowpan_skb_deliver(struct sk_buff *skb, struct ipv6hdr *hdr)
{
	struct sk_buff *new;
	struct lowpan_dev_record *entry;
	int stat = NET_RX_SUCCESS;

	new = skb_copy_expand(skb, sizeof(struct ipv6hdr), skb_tailroom(skb),
								GFP_ATOMIC);
	kfree_skb(skb);

	if (!new)
		return -ENOMEM;

	skb_push(new, sizeof(struct ipv6hdr));
	skb_reset_network_header(new);
	skb_copy_to_linear_data(new, hdr, sizeof(struct ipv6hdr));

	new->protocol = htons(ETH_P_IPV6);
	new->pkt_type = PACKET_HOST;

	rcu_read_lock();
	list_for_each_entry_rcu(entry, &lowpan_devices, list)
		if (lowpan_dev_info(entry->ldev)->real_dev == new->dev) {
			skb = skb_copy(new, GFP_ATOMIC);
			if (!skb) {
				stat = -ENOMEM;
				break;
			}

			skb->dev = entry->ldev;
			stat = netif_rx(skb);
		}
	rcu_read_unlock();

	kfree_skb(new);

	return stat;
}

static int
lowpan_process_data(struct sk_buff *skb)
{
	struct ipv6hdr hdr;
	u8 tmp, iphc0, iphc1, num_context = 0;
	u8 *_saddr, *_daddr;
	int err;

	lowpan_raw_dump_table(__func__, "raw skb data dump", skb->data,
				skb->len);
	/* at least two bytes will be used for the encoding */
	if (skb->len < 2)
		goto drop;
	iphc0 = lowpan_fetch_skb_u8(skb);
	iphc1 = lowpan_fetch_skb_u8(skb);

	_saddr = mac_cb(skb)->sa.hwaddr;
	_daddr = mac_cb(skb)->da.hwaddr;

	pr_debug("(%s): iphc0 = %02x, iphc1 = %02x\n", __func__, iphc0, iphc1);

	/* another if the CID flag is set */
	if (iphc1 & LOWPAN_IPHC_CID) {
		pr_debug("(%s): CID flag is set, increase header with one\n",
								__func__);
		if (!skb->len)
			goto drop;
		num_context = lowpan_fetch_skb_u8(skb);
	}

	hdr.version = 6;

	/* Traffic Class and Flow Label */
	switch ((iphc0 & LOWPAN_IPHC_TF) >> 3) {
	/*
	 * Traffic Class and FLow Label carried in-line
	 * ECN + DSCP + 4-bit Pad + Flow Label (4 bytes)
	 */
	case 0: /* 00b */
		if (!skb->len)
			goto drop;
		tmp = lowpan_fetch_skb_u8(skb);
		memcpy(&hdr.flow_lbl, &skb->data[0], 3);
		skb_pull(skb, 3);
		hdr.priority = ((tmp >> 2) & 0x0f);
		hdr.flow_lbl[0] = ((tmp >> 2) & 0x30) | (tmp << 6) |
					(hdr.flow_lbl[0] & 0x0f);
		break;
	/*
	 * Traffic class carried in-line
	 * ECN + DSCP (1 byte), Flow Label is elided
	 */
	case 2: /* 10b */
		if (!skb->len)
			goto drop;
		tmp = lowpan_fetch_skb_u8(skb);
		hdr.priority = ((tmp >> 2) & 0x0f);
		hdr.flow_lbl[0] = ((tmp << 6) & 0xC0) | ((tmp >> 2) & 0x30);
		hdr.flow_lbl[1] = 0;
		hdr.flow_lbl[2] = 0;
		break;
	/*
	 * Flow Label carried in-line
	 * ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided
	 */
	case 1: /* 01b */
		if (!skb->len)
			goto drop;
		tmp = lowpan_fetch_skb_u8(skb);
		hdr.flow_lbl[0] = (skb->data[0] & 0x0F) | ((tmp >> 2) & 0x30);
		memcpy(&hdr.flow_lbl[1], &skb->data[0], 2);
		skb_pull(skb, 2);
		break;
	/* Traffic Class and Flow Label are elided */
	case 3: /* 11b */
		hdr.priority = 0;
		hdr.flow_lbl[0] = 0;
		hdr.flow_lbl[1] = 0;
		hdr.flow_lbl[2] = 0;
		break;
	default:
		break;
	}

	/* Next Header */
	if ((iphc0 & LOWPAN_IPHC_NH_C) == 0) {
		/* Next header is carried inline */
		if (!skb->len)
			goto drop;
		hdr.nexthdr = lowpan_fetch_skb_u8(skb);
		pr_debug("(%s): NH flag is set, next header is carried "
			 "inline: %02x\n", __func__, hdr.nexthdr);
	}

	/* Hop Limit */
	if ((iphc0 & 0x03) != LOWPAN_IPHC_TTL_I)
		hdr.hop_limit = lowpan_ttl_values[iphc0 & 0x03];
	else {
		if (!skb->len)
			goto drop;
		hdr.hop_limit = lowpan_fetch_skb_u8(skb);
	}

	/* Extract SAM to the tmp variable */
	tmp = ((iphc1 & LOWPAN_IPHC_SAM) >> LOWPAN_IPHC_SAM_BIT) & 0x03;

	/* Source address uncompression */
	pr_debug("(%s): source address stateless compression\n", __func__);
	err = lowpan_uncompress_addr(skb, &hdr.saddr, lowpan_llprefix,
				lowpan_unc_llconf[tmp], skb->data);
	if (err)
		goto drop;

	/* Extract DAM to the tmp variable */
	tmp = ((iphc1 & LOWPAN_IPHC_DAM_11) >> LOWPAN_IPHC_DAM_BIT) & 0x03;

	/* check for Multicast Compression */
	if (iphc1 & LOWPAN_IPHC_M) {
		if (iphc1 & LOWPAN_IPHC_DAC) {
			pr_debug("(%s): destination address context-based "
				 "multicast compression\n", __func__);
			/* TODO: implement this */
		} else {
			u8 prefix[] = {0xff, 0x02};

			pr_debug("(%s): destination address non-context-based"
				 " multicast compression\n", __func__);
			if (0 < tmp && tmp < 3) {
				if (!skb->len)
					goto drop;
				else
					prefix[1] = lowpan_fetch_skb_u8(skb);
			}

			err = lowpan_uncompress_addr(skb, &hdr.daddr, prefix,
					lowpan_unc_mxconf[tmp], NULL);
			if (err)
				goto drop;
		}
	} else {
		pr_debug("(%s): destination address stateless compression\n",
								__func__);
		err = lowpan_uncompress_addr(skb, &hdr.daddr, lowpan_llprefix,
				lowpan_unc_llconf[tmp], skb->data);
		if (err)
			goto drop;
	}

	/* TODO: UDP header parse */

	/* Not fragmented package */
	hdr.payload_len = htons(skb->len);

	pr_debug("(%s): skb headroom size = %d, data length = %d\n", __func__,
						skb_headroom(skb), skb->len);

	pr_debug("(%s): IPv6 header dump:\n\tversion = %d\n\tlength  = %d\n\t"
		 "nexthdr = 0x%02x\n\thop_lim = %d\n", __func__, hdr.version,
		 ntohs(hdr.payload_len), hdr.nexthdr, hdr.hop_limit);

	lowpan_raw_dump_table(__func__, "raw header dump", (u8 *)&hdr,
							sizeof(hdr));
	return lowpan_skb_deliver(skb, &hdr);
drop:
	kfree_skb(skb);
	return -EINVAL;
}

static int lowpan_set_address(struct net_device *dev, void *p)
{
	struct sockaddr *sa = p;

	if (netif_running(dev))
		return -EBUSY;

	/* TODO: validate addr */
	memcpy(dev->dev_addr, sa->sa_data, dev->addr_len);

	return 0;
}

static netdev_tx_t lowpan_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int err = 0;

	pr_debug("(%s): package xmit\n", __func__);

	skb->dev = lowpan_dev_info(dev)->real_dev;
	if (skb->dev == NULL) {
		pr_debug("(%s) ERROR: no real wpan device found\n", __func__);
		dev_kfree_skb(skb);
	} else
		err = dev_queue_xmit(skb);

	return (err < 0 ? NETDEV_TX_BUSY : NETDEV_TX_OK);
}

static void lowpan_dev_free(struct net_device *dev)
{
	dev_put(lowpan_dev_info(dev)->real_dev);
	free_netdev(dev);
}

static struct header_ops lowpan_header_ops = {
	.create	= lowpan_header_create,
};

static const struct net_device_ops lowpan_netdev_ops = {
	.ndo_start_xmit		= lowpan_xmit,
	.ndo_set_mac_address	= lowpan_set_address,
};

static void lowpan_setup(struct net_device *dev)
{
	pr_debug("(%s)\n", __func__);

	dev->addr_len		= IEEE802154_ADDR_LEN;
	memset(dev->broadcast, 0xff, IEEE802154_ADDR_LEN);
	dev->type		= ARPHRD_IEEE802154;
	dev->features		= NETIF_F_NO_CSUM;
	/* Frame Control + Sequence Number + Address fields + Security Header */
	dev->hard_header_len	= 2 + 1 + 20 + 14;
	dev->needed_tailroom	= 2; /* FCS */
	dev->mtu		= 1281;
	dev->tx_queue_len	= 0;
	dev->flags		= IFF_NOARP | IFF_BROADCAST;
	dev->watchdog_timeo	= 0;

	dev->netdev_ops		= &lowpan_netdev_ops;
	dev->header_ops		= &lowpan_header_ops;
	dev->destructor		= lowpan_dev_free;
}

static int lowpan_validate(struct nlattr *tb[], struct nlattr *data[])
{
	pr_debug("(%s)\n", __func__);

	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != IEEE802154_ADDR_LEN)
			return -EINVAL;
	}
	return 0;
}

static int lowpan_rcv(struct sk_buff *skb, struct net_device *dev,
	struct packet_type *pt, struct net_device *orig_dev)
{
	if (!netif_running(dev))
		goto drop;

	if (dev->type != ARPHRD_IEEE802154)
		goto drop;

	/* check that it's our buffer */
	if ((skb->data[0] & 0xe0) == 0x60)
		lowpan_process_data(skb);

	return NET_RX_SUCCESS;

drop:
	kfree_skb(skb);
	return NET_RX_DROP;
}

static int lowpan_newlink(struct net *src_net, struct net_device *dev,
			  struct nlattr *tb[], struct nlattr *data[])
{
	struct net_device *real_dev;
	struct lowpan_dev_record *entry;

	pr_debug("(%s)\n", __func__);

	if (!tb[IFLA_LINK])
		return -EINVAL;
	/* find and hold real wpan device */
	real_dev = dev_get_by_index(src_net, nla_get_u32(tb[IFLA_LINK]));
	if (!real_dev)
		return -ENODEV;

	lowpan_dev_info(dev)->real_dev = real_dev;
	mutex_init(&lowpan_dev_info(dev)->dev_list_mtx);

	entry = kzalloc(sizeof(struct lowpan_dev_record), GFP_KERNEL);
	if (!entry) {
		dev_put(real_dev);
		lowpan_dev_info(dev)->real_dev = NULL;
		return -ENOMEM;
	}

	entry->ldev = dev;

	mutex_lock(&lowpan_dev_info(dev)->dev_list_mtx);
	INIT_LIST_HEAD(&entry->list);
	list_add_tail(&entry->list, &lowpan_devices);
	mutex_unlock(&lowpan_dev_info(dev)->dev_list_mtx);

	register_netdevice(dev);

	return 0;
}

static void lowpan_dellink(struct net_device *dev, struct list_head *head)
{
	struct lowpan_dev_info *lowpan_dev = lowpan_dev_info(dev);
	struct net_device *real_dev = lowpan_dev->real_dev;
	struct lowpan_dev_record *entry;
	struct lowpan_dev_record *tmp;

	ASSERT_RTNL();

	mutex_lock(&lowpan_dev_info(dev)->dev_list_mtx);
	list_for_each_entry_safe(entry, tmp, &lowpan_devices, list) {
		if (entry->ldev == dev) {
			list_del(&entry->list);
			kfree(entry);
		}
	}
	mutex_unlock(&lowpan_dev_info(dev)->dev_list_mtx);

	mutex_destroy(&lowpan_dev_info(dev)->dev_list_mtx);

	unregister_netdevice_queue(dev, head);

	dev_put(real_dev);
}

static struct rtnl_link_ops lowpan_link_ops __read_mostly = {
	.kind		= "lowpan",
	.priv_size	= sizeof(struct lowpan_dev_info),
	.setup		= lowpan_setup,
	.newlink	= lowpan_newlink,
	.dellink	= lowpan_dellink,
	.validate	= lowpan_validate,
};

static inline int __init lowpan_netlink_init(void)
{
	return rtnl_link_register(&lowpan_link_ops);
}

static inline void __init lowpan_netlink_fini(void)
{
	rtnl_link_unregister(&lowpan_link_ops);
}

static struct packet_type lowpan_packet_type = {
	.type = __constant_htons(ETH_P_IEEE802154),
	.func = lowpan_rcv,
};

static int __init lowpan_init_module(void)
{
	int err = 0;

	pr_debug("(%s)\n", __func__);

	err = lowpan_netlink_init();
	if (err < 0)
		goto out;

	dev_add_pack(&lowpan_packet_type);
out:
	return err;
}

static void __exit lowpan_cleanup_module(void)
{
	pr_debug("(%s)\n", __func__);

	lowpan_netlink_fini();

	dev_remove_pack(&lowpan_packet_type);
}

module_init(lowpan_init_module);
module_exit(lowpan_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("lowpan");
