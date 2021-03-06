/*
 * \brief  Dummies
 * \author Stefan Kalkowski
 * \author Sebastian Sumpf
 * \date   2020-12-02
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <lx_emul.h>

#if 0
#define TRACE \
	do { \
		lx_printf("%s not implemented from %p\n", __func__, __builtin_return_address(0)); \
	} while (0)
#else
#define TRACE do { ; } while (0)
#endif

#define TRACE_AND_STOP \
	do { \
		lx_printf("%s not implemented from %p\n", __func__, __builtin_return_address(0)); \
		BUG(); \
	} while (0)

struct page *alloc_pages(gfp_t gfp_mask, unsigned int order)
{
	TRACE_AND_STOP;
	return NULL;
}

u16 bitrev16(u16 in)
{
	TRACE_AND_STOP;
	return -1;
}

u16 crc16(u16 crc, const u8 *buffer, size_t len)
{
	TRACE_AND_STOP;
	return -1;
}

__sum16 csum_fold(__wsum csum)
{
	TRACE_AND_STOP;
	return -1;
}

__wsum csum_partial(const void *buff, int len, __wsum sum)
{
	TRACE_AND_STOP;
	return -1;
}

void dev_hold(struct net_device *dev)
{
	TRACE_AND_STOP;
}

void dev_put(struct net_device *dev)
{
	TRACE_AND_STOP;
}

const char *dev_name(const struct device *dev)
{
	TRACE;
	return NULL;
}

int device_set_wakeup_enable(struct device *dev, bool enable)
{
	TRACE_AND_STOP;
	return -1;
}


void dst_release(struct dst_entry *dst)
{
	TRACE_AND_STOP;
}

struct net_device;
u32 ethtool_op_get_link(struct net_device *dev)
{
	TRACE_AND_STOP;
	return -1;
}

int ethtool_op_get_ts_info(struct net_device *dev, struct ethtool_ts_info *eti)
{
	TRACE_AND_STOP;
	return -1;
}

bool file_ns_capable(const struct file *file, struct user_namespace *ns, int cap)
{
	TRACE_AND_STOP;
	return -1;
}

void free_netdev(struct net_device * ndev)
{
	TRACE_AND_STOP;
}

void free_uid(struct user_struct * user)
{
	TRACE_AND_STOP;
}

struct mii_if_info;
int generic_mii_ioctl(struct mii_if_info *mii_if, struct mii_ioctl_data *mii_data, int cmd, unsigned int *duplex_changed)
{
	TRACE_AND_STOP;
	return -1;
}

void get_page(struct page *page)
{
	TRACE_AND_STOP;
}

bool gfpflags_allow_blocking(const gfp_t gfp_flags)
{
	TRACE_AND_STOP;
	return -1;
}

bool gfp_pfmemalloc_allowed(gfp_t gfp_flags)
{
	TRACE_AND_STOP;
	return -1;
}

int hex2bin(u8 *dst, const char *src, size_t count)
{
	TRACE_AND_STOP;
	return -1;
}

int in_irq()
{
	TRACE_AND_STOP;
	return -1;
}

void *kmap_atomic(struct page *page)
{
	TRACE_AND_STOP;
	return NULL;
}

int memcmp(const void *s1, const void *s2, size_t size)
{
	TRACE_AND_STOP;
	return -1;
}

void mii_ethtool_get_link_ksettings( struct mii_if_info *mii, struct ethtool_link_ksettings *cmd)
{
	TRACE_AND_STOP;
}

int mii_ethtool_set_link_ksettings( struct mii_if_info *mii, const struct ethtool_link_ksettings *cmd)
{
	TRACE_AND_STOP;
	return -1;
}

unsigned netdev_mc_count(struct net_device * dev)
{
	TRACE;
	return 1;
}

int netdev_mc_empty(struct net_device * ndev)
{
	TRACE;
	return 1;
}

void netdev_stats_to_stats64(struct rtnl_link_stats64 *stats64, const struct net_device_stats *netdev_stats)
{
	TRACE_AND_STOP;
}

bool netdev_uses_dsa(struct net_device *dev)
{
	TRACE;
	return false;
}

void netif_device_attach(struct net_device *dev)
{
	TRACE;
}

void netif_device_detach(struct net_device *dev)
{
	TRACE_AND_STOP;
}

int  netif_device_present(struct net_device * d)
{
	TRACE;
	return 1;
}

u32 netif_msg_init(int arg0, int arg1)
{
	TRACE;
	return 0;
}

void netif_start_queue(struct net_device *dev)
{
	TRACE;
}

void netif_stop_queue(struct net_device *dev)
{
	TRACE;
}

void netif_trans_update(struct net_device *dev)
{
	TRACE;
}

void netif_tx_wake_all_queues(struct net_device *dev)
{
	TRACE_AND_STOP;
}

void netif_wake_queue(struct net_device * d)
{
	TRACE;
}

void netif_tx_lock_bh(struct net_device *dev)
{
	TRACE;
}

void netif_tx_unlock_bh(struct net_device *dev)
{
	TRACE;
}

const void *of_get_mac_address(struct device_node *np)
{
	TRACE;
	return NULL;
}

void pm_runtime_enable(struct device *dev)
{
	TRACE;
}

void read_lock_bh(rwlock_t * l)
{
	TRACE_AND_STOP;
}

void read_unlock_bh(rwlock_t * l)
{
	TRACE_AND_STOP;
}

void unregister_netdev(struct net_device * dev)
{
	TRACE_AND_STOP;
}

void secpath_reset(struct sk_buff * skb)
{
	TRACE;
}

void __set_current_state(int state)
{
	TRACE_AND_STOP;
}

void sg_init_table(struct scatterlist *sg, unsigned int arg)
{
	TRACE_AND_STOP;
}

void sg_set_buf(struct scatterlist *sg, const void *buf, unsigned int buflen)
{
	TRACE_AND_STOP;
}

void sg_set_page(struct scatterlist *sg, struct page *page, unsigned int len, unsigned int offset)
{
	TRACE_AND_STOP;
}

void sk_free(struct sock *sk)
{
	TRACE_AND_STOP;
}

void sock_efree(struct sk_buff *skb)
{
	TRACE_AND_STOP;
}

void spin_lock(spinlock_t *lock)
{
	TRACE;
}

void spin_lock_irq(spinlock_t *lock)
{
	TRACE;
}

void spin_lock_nested(spinlock_t *lock, int subclass)
{
	TRACE;
}

void spin_unlock_irq(spinlock_t *lock)
{
	TRACE;
}

void spin_lock_bh(spinlock_t *lock)
{
	TRACE;
}

void spin_unlock_bh(spinlock_t *lock)
{
	TRACE;
}

size_t strlcpy(char *dest, const char *src, size_t size)
{
	TRACE_AND_STOP;
	return -1;
}

void tasklet_kill(struct tasklet_struct *t)
{
	TRACE_AND_STOP;
}

void trace_consume_skb(struct sk_buff *skb)
{
	TRACE;
}

void trace_kfree_skb(struct sk_buff *skb, void *arg)
{
	TRACE;
}

unsigned int u64_stats_fetch_begin_irq(const struct u64_stats_sync *p)
{
	TRACE_AND_STOP;
	return -1;
}

bool u64_stats_fetch_retry_irq(const struct u64_stats_sync *p, unsigned int s)
{
	TRACE_AND_STOP;
	return -1;
}

struct usb_device;
int usb_clear_halt(struct usb_device *dev, int pipe)
{
	TRACE_AND_STOP;
	return -1;
}

struct usb_driver;
struct usb_interface;
void usb_driver_release_interface(struct usb_driver *driver, struct usb_interface *iface)
{
	TRACE_AND_STOP;
}

struct usb_anchor;
struct urb *usb_get_from_anchor(struct usb_anchor *anchor)
{
	TRACE_AND_STOP;
	return NULL;
}

struct urb *usb_get_urb(struct urb *urb)
{
	TRACE_AND_STOP;
	return NULL;
}

int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe, void *data, int len, int *actual_length, int timeout)
{
	TRACE_AND_STOP;
	return -1;
}

void usb_scuttle_anchored_urbs(struct usb_anchor *anchor)
{
	TRACE_AND_STOP;
}

int usb_string(struct usb_device *dev, int index, char *buf, size_t size)
{
	TRACE_AND_STOP;
	return -1;
}

const struct usb_device_id *usb_match_id(struct usb_interface *interface,
                                         const struct usb_device_id *id)
{
	TRACE_AND_STOP;
	return NULL;
}

struct usb_host_interface *usb_altnum_to_altsetting(
	const struct usb_interface *intf, unsigned int altnum)
{
	TRACE_AND_STOP;
	return NULL;
}

ktime_t ktime_get_real(void)
{
	TRACE_AND_STOP;
	return -1;
}

void kunmap_atomic(void *addr)
{
	TRACE_AND_STOP;
}

int kstrtoul(const char *s, unsigned int base, unsigned long *res)
{
	TRACE_AND_STOP;
	return -1;
}

void might_sleep()
{
	TRACE_AND_STOP;
}

bool page_is_pfmemalloc(struct page *page)
{
	TRACE_AND_STOP;
	return -1;
}

void put_page(struct page *page)
{
	TRACE_AND_STOP;
}

void usb_kill_urb(struct urb *urb)
{
	TRACE_AND_STOP;
}

int usb_unlink_urb(struct urb *urb)
{
	TRACE_AND_STOP;
	return -1;
}

void usleep_range(unsigned long min, unsigned long max)
{
	TRACE;
}

void phy_stop(struct phy_device *phydev)
{
	TRACE;
}

void phy_disconnect(struct phy_device *phydev)
{
	TRACE;
}

void phy_print_status(struct phy_device *phydev)
{
	TRACE;
}

int phy_mii_ioctl(struct phy_device *phydev, struct ifreq *ifr, int cmd)
{
	TRACE;
	return 0;
}

typedef int phy_interface_t;
struct phy_device * phy_connect(struct net_device *dev, const char *bus_id, void (*handler)(struct net_device *), phy_interface_t interface)
{
	TRACE;
	return 0;
}

int  genphy_resume(struct phy_device *phydev) { TRACE; return 0; }

void phy_start(struct phy_device *phydev) { TRACE; }

struct mii_bus;
void mdiobus_free(struct mii_bus *bus) { TRACE; }

void mdiobus_unregister(struct mii_bus *bus) { TRACE; }

struct mii_bus *mdiobus_alloc_size(size_t size)
{
	TRACE_AND_STOP;
	return NULL;
}

int __mdiobus_register(struct mii_bus *bus, struct module *owner)
{
	TRACE_AND_STOP;
	return -1;
}

int phy_ethtool_get_link_ksettings(struct net_device *ndev, struct ethtool_link_ksettings *cmd)
{
	TRACE_AND_STOP;
	return -1;
}

int phy_ethtool_set_link_ksettings(struct net_device *ndev, const struct ethtool_link_ksettings *cmd)
{
	TRACE_AND_STOP;
	return -1;
}

int phy_ethtool_nway_reset(struct net_device *ndev)
{
	TRACE_AND_STOP;
	return -1;
}

int __vlan_get_tag(const struct sk_buff *skb, u16 *vlan_tci)
{
	TRACE;
	return -1;
}

void __vlan_hwaccel_put_tag(struct sk_buff *skb, __be16 vlan_proto, u16 vlan_tci)
{
	TRACE;
}

struct vlan_ethhdr *vlan_eth_hdr(const struct sk_buff *skb)
{
	TRACE_AND_STOP;
	return NULL;
}


struct net_device *
__vlan_find_dev_deep_rcu(struct net_device *real_dev, __be16 vlan_proto, u16 vlan_id)
{
	TRACE_AND_STOP;
	return NULL;
}

bool ipv6_addr_is_solict_mult(const struct in6_addr *addr)
{
	TRACE;
	return false;
}

int ipv6_addr_type(const struct in6_addr *addr)
{
	TRACE;
	return -1;
}

struct inet6_dev *in6_dev_get(const struct net_device *dev)
{
	TRACE_AND_STOP;
	return NULL;
}

void in6_dev_put(struct inet6_dev *idev)
{
	TRACE_AND_STOP;
}


struct usb_class_driver;
void usb_deregister_dev(struct usb_interface *intf,struct usb_class_driver *class_driver)
{
	TRACE_AND_STOP;
}

void poll_wait(struct file *f, wait_queue_head_t *w, poll_table *p)
{
	TRACE_AND_STOP;
}

loff_t noop_llseek(struct file *file, loff_t offset, int whence)
{
	TRACE_AND_STOP;
	return 0;
}

int sysctl_tstamp_allow_data;
struct user_namespace init_user_ns;
const struct ipv6_stub *ipv6_stub;
