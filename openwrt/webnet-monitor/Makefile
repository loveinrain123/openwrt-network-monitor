include $(TOPDIR)/rules.mk

PKG_NAME:=webnet-monitor
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=libpcap

include $(INCLUDE_DIR)/package.mk

define Package/webnet-monitor
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Lightweight libpcap traffic monitor for OpenWrt
  DEPENDS:=+libpcap
endef

define Package/webnet-monitor/description
  Capture IPv4 packets with libpcap, aggregate traffic by IP, print
  TX/RX/Packets/Peak values, and write results to a JSON file for the web UI.
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/monitor.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/webnet-monitor \
		$(PKG_BUILD_DIR)/monitor.c -lpcap
endef

define Package/webnet-monitor/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/webnet-monitor $(1)/usr/bin/webnet-monitor
endef

$(eval $(call BuildPackage,webnet-monitor))
