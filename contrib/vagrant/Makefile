# === Makefile ------------------------------------------------------------===
# Copyright © 2011-2012 Bitcoin Developers.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# ===----------------------------------------------------------------------===

MAKEFILE := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

SHELL    := $(shell bash --help >/dev/null 2>&1 && echo ba)sh

GREP     := $(shell egrep --help >/dev/null 2>&1 && echo e)grep
PATCH    := $(shell gpatch --help >/dev/null 2>&1 && echo g)patch
SED      := $(shell gsed --help >/dev/null 2>&1 && echo g)sed
WGET     := wget --no-check-certificate \
                 --user-agent=$(shell wget --version | \
                 $(SED) -n 's,GNU \(Wget\) \([0-9.]*\).*,\1/\2,p')

REQUIREMENTS := $(SHELL) bzip2 cut diff $(GREP) gzip $(MAKE) openssl $(PATCH) \
                ruby $(SED) sort tar vagrant wget xargs xz

BUILD_MANIFEST = cd '$(shell dirname '$(MAKEFILE)')'/../.. && \
	find $(1) -print0 | \
	xargs -0 -n1 openssl sha256 2>/dev/null | \
	openssl sha256 | \
	$(SED) -n 's,^.*\([0-9a-f]\{64\}\),\1,p'

VALIDATE_MANIFEST = \
	if [ -f "$(1)" ] && echo "$(2)" | diff "$(1)" - 2>&1 >/dev/null; then \
	    rm -f "$(1)"; \
	fi

# ===----------------------------------------------------------------------===

PKGS            :=
CACHE_DIR       := cache
HOST_CACHE_DIR  := $(CACHE_DIR)
GUEST_CACHE_DIR := /vagrant/$(CACHE_DIR)

SOURCEFORGE_MIRROR := kent.dl.sourceforge.net

PKG             := boost
$(PKG)_VERSION  := 1.47.0
$(PKG)_CHECKSUM := 815a5d9faac4dbd523fbcf3fe1065e443c0bbf43427c44aa423422c6ec4c2e31
$(PKG)_SUBDIR   := boost_$(subst .,_,$($(PKG)_VERSION))
$(PKG)_FILE     := boost_$(subst .,_,$($(PKG)_VERSION)).tar.bz2
$(PKG)_URL      := http://$(SOURCEFORGE_MIRROR)/project/boost/boost/$($(PKG)_VERSION)/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := boost-win32
$(PKG)_VERSION  := 1.49.0
$(PKG)_CHECKSUM := dd748a7f5507a7e7af74f452e1c52a64e651ed1f7263fce438a06641d2180d3c
$(PKG)_SUBDIR   := boost_$(subst .,_,$($(PKG)_VERSION))
$(PKG)_FILE     := boost_$(subst .,_,$($(PKG)_VERSION)).tar.bz2
$(PKG)_URL      := http://$(SOURCEFORGE_MIRROR)/project/boost/boost/$($(PKG)_VERSION)/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := db
$(PKG)_VERSION  := 4.8.30
$(PKG)_CHECKSUM := 12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef
$(PKG)_SUBDIR   := $(PKG)-$($(PKG)_VERSION).NC
$(PKG)_FILE     := $(PKG)-$($(PKG)_VERSION).NC.tar.gz
$(PKG)_URL      := http://download.oracle.com/berkeley-db/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := libpng
$(PKG)_VERSION  := 1.5.9
$(PKG)_CHECKSUM := b75dae26151f9b031062c8d2f577a094b08da0ae44fe8c11175d0b9ff434cc02
$(PKG)_SUBDIR   := $(PKG)-$($(PKG)_VERSION)
$(PKG)_FILE     := $(PKG)-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := ftp://ftp.simplesystems.org/pub/$(PKG)/png/src/$($(PKG)_FILE)
$(PKG)_URL_2    := http://image_magick.veidrodis.com/image_magick/delegates/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := miniupnpc
$(PKG)_VERSION  := 1.6
$(PKG)_CHECKSUM := bbd6b756e6af44b5a5b0f9b93eada3fb8922ed1d6451b7d6f184d0ae0c813994
$(PKG)_SUBDIR   := $(PKG)-$($(PKG)_VERSION)
$(PKG)_FILE     := $(PKG)-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := http://miniupnp.free.fr/files/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := openssl
$(PKG)_VERSION  := 1.0.1b
$(PKG)_CHECKSUM := 1187bf2d5cdf0b286b951bf5c777aa97855cd7db24fdb9604cb557c8b1d7364b
$(PKG)_SUBDIR   := $(PKG)-$($(PKG)_VERSION)
$(PKG)_FILE     := $(PKG)-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := http://www.openssl.org/source/$($(PKG)_FILE)
$(PKG)_URL_2    := ftp://ftp.openssl.org/source/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := qrencode
$(PKG)_VERSION  := 3.2.0
$(PKG)_CHECKSUM := 03c4bc7cd9a75747c3815d509bbe061907d615764f2357923f0db948c567068f
$(PKG)_SUBDIR   := $(PKG)-$($(PKG)_VERSION)
$(PKG)_FILE     := $(PKG)-$($(PKG)_VERSION).tar.bz2
$(PKG)_URL      := http://fukuchi.org/works/$(PKG)/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := qt
$(PKG)_VERSION  := 4.7.4
$(PKG)_CHECKSUM := 97195ebce8a46f9929fb971d9ae58326d011c4d54425389e6e936514f540221e
$(PKG)_SUBDIR   := $(PKG)-everywhere-opensource-src-$($(PKG)_VERSION)
$(PKG)_FILE     := $(PKG)-everywhere-opensource-src-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := http://releases.qt-project.org/qt4/source/$($(PKG)_FILE)
$(PKG)_URL_2    := http://get.qt.nokia.com/qt/source/$($(PKG)_FILE)
PKGS            += $(PKG)

PKG             := zlib
$(PKG)_VERSION  := 1.2.6
$(PKG)_CHECKSUM := 21235e08552e6feba09ea5e8d750805b3391c62fb81c71a235c0044dc7a8a61b
$(PKG)_SUBDIR   := $(PKG)-$($(PKG)_VERSION)
$(PKG)_FILE     := $(PKG)-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := http://zlib.net/$($(PKG)_FILE)
$(PKG)_URL_2    := http://$(SOURCEFORGE_MIRROR)/project/libpng/$(PKG)/$($(PKG)_VERSION)/$($(PKG)_FILE)
PKGS            += $(PKG)

# ===----------------------------------------------------------------------===

VAGRANT_UP = \
	vagrant up

VAGRANT_HALT = \
	vagrant halt

VAGRANT_RELOAD = \
	vagrant reload

VAGRANT_DESTROY = \
	vagrant destroy --force

# ===----------------------------------------------------------------------===

.PHONY: all
all: bitcoin-linux bitcoin-win32

.PHONY: mostlyclean
mostlyclean:

.PHONY: clean
clean: mostlyclean
	-rm -rf output
	-rm -f .stamp-*
	-$(call VAGRANT_DESTROY)

.PHONY: distclean
distclean: clean
	-rm -rf cache

.PHONY: maintainer-clean
maintainer-clean: distclean
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'

.PHONY: dist
dist: all

# ===----------------------------------------------------------------------===

.PHONY: check-requirements
define CHECK_REQUIREMENT
	@if ! $(1) --help &>/dev/null; then \
	    if ! which $(1) &>/dev/null; then \
	        echo; \
	        echo 'Missing requirement: $(1)'; \
	        echo; \
	        echo 'Please have a look at "README" to ensure'; \
	        echo 'that your system meets all requirements.'; \
	        echo; \
	        exit 1; \
	    fi; \
	fi

endef
define CHECK_REQUIREMENT_VERSION
	@if ! $(1) --version | head -1 | grep ' \($(2)\)$$' >/dev/null; then \
	    echo; \
	    echo 'Wrong version of requirement: $(1)'; \
	    echo; \
	    echo 'Please have a look at "README" to ensure'; \
	    echo 'that your system meets all requirements.'; \
	    echo; \
	    exit 1; \
	fi

endef
check-requirements: .stamp-check-requirements.h
.stamp-check-requirements.h: $(MAKEFILE)
	@echo '[check requirements]'
	$(foreach REQUIREMENT,$(REQUIREMENTS),$(call CHECK_REQUIREMENT,$(REQUIREMENT)))
	$(call CHECK_REQUIREMENT_VERSION,vagrant,1\.[0-9]\.[0-9])
	@touch "$@"

# ===----------------------------------------------------------------------===

.PHONY: download
download: $(addprefix download-,$(PKGS))

PKG_CHECKSUM = \
	openssl sha256 '$(HOST_CACHE_DIR)/$($(1)_FILE)' 2>/dev/null | $(SED) -n 's,^.*\([0-9a-f]\{64\}\)$$,\1,p'

CHECK_PKG_ARCHIVE = \
	[ x'$($(1)_CHECKSUM)' == x"`$$(call PKG_CHECKSUM,$(1))`" ]

DOWNLOAD_PKG_ARCHIVE = \
	mkdir -p '$(HOST_CACHE_DIR)' && \
	$(if $($(1)_URL_2), \
	    ( $(WGET) -T 30 -t 3 -O- '$($(1)_URL)' || $(WGET) -O- '$($(1)_URL_2)' ), \
	    $(WGET) -O- '$($(1)_URL)') \
	$(if $($(1)_FIX_GZIP), \
	    | gzip -d | gzip -9n, \
	    ) \
	> '$(HOST_CACHE_DIR)/$($(1)_FILE)'

define PKG_RULE
.PHONY: download-$(1)
download-$(1): $(HOST_CACHE_DIR)/$($(1)_FILE)
$(HOST_CACHE_DIR)/$($(1)_FILE): .stamp-check-requirements.h
	@echo -n "Checking cached package $(1)-$($(1)_VERSION)... "; \
	if ! $(call CHECK_PKG_ARCHIVE,$(1)); then \
		echo "failed"; \
	     $(call DOWNLOAD_PKG_ARCHIVE,$(1)); \
	     $(call CHECK_PKG_ARCHIVE,$(1)) \
	        || { echo 'Wrong checksum!'; exit 1; }; \
	else \
		echo "ok"; \
	fi

endef
$(foreach PKG,$(PKGS),$(eval $(call PKG_RULE,$(PKG))))

# ===----------------------------------------------------------------------===

HOST_VM_FILES    := contrib/vagrant/Makefile \
                    contrib/vagrant/Vagrantfile \
                    contrib/vagrant/patch/host-vm-etc-network-interfaces-100-add_lxc_br0.patch \
                    contrib/vagrant/patch/host-vm-etc-sysctl.conf-100-enable_ipv4_forward.patch
HOST_VM_FILES    := $(sort $(HOST_VM_FILES))
HOST_VM_MANIFEST := $(shell $(call BUILD_MANIFEST,$(HOST_VM_FILES)))

_ := $(shell $(call \
	VALIDATE_MANIFEST, \
	.stamp-host-vm.manifest, \
	$(HOST_VM_MANIFEST) \
))

.PHONY: host-vm
host-vm: .stamp-host-vm.manifest
.stamp-host-vm.manifest: \
	.stamp-check-requirements.h \
	$(foreach FILE,$(HOST_VM_FILES),$(shell echo $(FILE) | $(SED) 's:^contrib/vagrant/::g'))
	
	-$(call VAGRANT_DESTROY)
	
	$(call VAGRANT_UP)
	
	vagrant ssh --command "sudo apt-get update"
	vagrant ssh --command "sudo apt-get upgrade -y"
	$(call VAGRANT_RELOAD)
	
	vagrant ssh --command "sudo apt-get install -y \
	    debootstrap git lxc python-vm-builder ruby patch zip"
	
	vagrant ssh --command "sudo apt-get install -y bridge-utils"
	vagrant ssh --command "sudo brctl addbr br0"
	vagrant ssh --command "sudo $(SHELL) -c 'cd /etc/network && \
	    patch -p1 < /vagrant/patch/host-vm-etc-network-interfaces-100-add_lxc_br0.patch'"
	vagrant ssh --command "sudo $(SHELL) -c 'cd /etc && \
	    patch -p1 < /vagrant/patch/host-vm-etc-sysctl.conf-100-enable_ipv4_forward.patch'"
	
	vagrant ssh --command "sudo apt-get install -y apt-cacher-ng"
	
	-$(call VAGRANT_HALT)
	
	echo $(HOST_VM_MANIFEST) > "$@"

# ===----------------------------------------------------------------------===

GITIAN_VM_FILES    := contrib/vagrant/Makefile \
                      contrib/vagrant/patch/gitian-builder-69bd6a53-100-change_ip.patch \
                      contrib/vagrant/patch/gitian-builder-69bd6a53-200-removepkg_cron.patch \
                      contrib/vagrant/patch/gitian-builder-69bd6a53-300-external_mirrors.patch \
                      contrib/vagrant/patch/gitian-builder-69bd6a53-400-ssh_key.patch
GITIAN_VM_FILES    := $(sort $(GITIAN_VM_FILES))
GITIAN_VM_MANIFEST := $(shell $(call BUILD_MANIFEST,$(GITIAN_VM_FILES)))

_ := $(shell $(call \
	VALIDATE_MANIFEST, \
	.stamp-gitian-guest-vm.manifest, \
	$(GITIAN_VM_MANIFEST) \
))

.PHONY: gitian-guest-vm
gitian-guest-vm: .stamp-gitian-guest-vm.manifest
.stamp-gitian-guest-vm.manifest: \
	.stamp-host-vm.manifest \
	$(foreach FILE,$(GITIAN_VM_FILES),$(shell echo $(FILE) | $(SED) 's:^contrib/vagrant/::g'))
	
	$(call VAGRANT_UP)
	
	vagrant ssh --command "\
	    if [ -d ~/gitian-builder ]; then \
	        rm -rf ~/gitian-builder; \
	    fi"
	vagrant ssh --command "\
	    git clone 'https://github.com/devrandom/gitian-builder' ~/gitian-builder"
	vagrant ssh --command "cd ~/gitian-builder && \
	    git checkout 69bd6a53"
	vagrant ssh --command "cd ~/gitian-builder && \
	    patch -p1 < /vagrant/patch/gitian-builder-69bd6a53-100-change_ip.patch"
	vagrant ssh --command "cd ~/gitian-builder && \
	    patch -p1 < /vagrant/patch/gitian-builder-69bd6a53-200-removepkg_cron.patch"
	vagrant ssh --command "cd ~/gitian-builder && \
	    patch -p1 < /vagrant/patch/gitian-builder-69bd6a53-300-external_mirrors.patch"
	vagrant ssh --command "cd ~/gitian-builder && \
	    patch -p1 < /vagrant/patch/gitian-builder-69bd6a53-400-ssh_key.patch"
	vagrant ssh --command "cd ~/gitian-builder && \
	    ln -s $(GUEST_CACHE_DIR) inputs"
	
	vagrant ssh --command "\
	    cd ~/gitian-builder; \
	    if [ -e $(GUEST_CACHE_DIR)/gitian-guest-vm/$(GITIAN_VM_MANIFEST) ]; then \
	        rsync -av $(GUEST_CACHE_DIR)/gitian-guest-vm/$(GITIAN_VM_MANIFEST)/base-* . || exit 1; \
	    else \
	        bin/make-base-vm --lxc --suite lucid --arch i386 || exit 1; \
	        bin/make-base-vm --lxc --suite lucid --arch amd64 || exit 1; \
	        mkdir -p $(GUEST_CACHE_DIR)/gitian-guest-vm/$(GITIAN_VM_MANIFEST) || exit 1; \
	        rsync -av base-* $(GUEST_CACHE_DIR)/gitian-guest-vm/$(GITIAN_VM_MANIFEST) || \
	            rm -rf $(GUEST_CACHE_DIR)/gitian-guest-vm/$(GITIAN_VM_MANIFEST); \
	        [ -e $(GUEST_CACHE_DIR)/gitian-guest-vm/$(GITIAN_VM_MANIFEST) ] || \
	            exit 1; \
	    fi"
	
	-$(call VAGRANT_HALT)
	
	echo $(GITIAN_VM_MANIFEST) > "$@"

# ===----------------------------------------------------------------------===

ifeq ($(shell git log -1 2>&1 >/dev/null && echo true),true)
SOURCE_FILES := $(shell \
	git ls-tree --name-only --full-tree -r `git log -1 --format="%H"` \
	    | $(GREP) -v '^contrib/vagrant/' \
	    | sort)
else
SOURCE_FILES := $(shell cd ../.. && \
	find -L . -type f -print \
	    | $(SED) 's:^\./::g' \
	    | $(GREP) -v '^\.git/' \
	    | $(GREP) -v '^contrib/vagrant/' \
	    | sort)
endif

SOURCE_MANIFEST := $(shell $(call BUILD_MANIFEST,$(SOURCE_FILES)))

_ := $(shell $(call \
	VALIDATE_MANIFEST, \
	$(HOST_CACHE_DIR)/src-local.manifest, \
	$(SOURCE_MANIFEST) \
))

.PHONY: bundle-source
bundle-source: $(HOST_CACHE_DIR)/src-local.manifest
$(HOST_CACHE_DIR)/src-local.manifest: $(HOST_CACHE_DIR)/src-local.tar.xz
	echo $(SOURCE_MANIFEST) >'$@'
$(HOST_CACHE_DIR)/src-local.tar.xz: \
	$(foreach FILE,$(SOURCE_FILES),$(shell echo $(FILE) | $(SED) 's:^:../../:g'))
	
	mkdir -p '$(HOST_CACHE_DIR)'
	rm -f '$(HOST_CACHE_DIR)'/src-local.{tar,tar.xz}
	COPYFILE_DISABLE=1 tar -C ../.. -rf '$(HOST_CACHE_DIR)'/src-local.tar $(SOURCE_FILES)
	xz '$(HOST_CACHE_DIR)'/src-local.tar

# ===----------------------------------------------------------------------===

OUTPUT_DIR         := output
HOST_OUTPUT_DIR    := $(OUTPUT_DIR)
GUEST_OUTPUT_DIR   := /vagrant/$(OUTPUT_DIR)
PATCH_DIR          := patch
HOST_PATCH_DIR     := $(PATCH_DIR)
GUEST_PATCH_DIR    := /vagrant/$(PATCH_DIR)

TARGETS            :=

TARGET             := bitcoin-deps-win32
$(TARGET)_NAME     := bitcoin-deps
$(TARGET)_VERSION  := 0.0.4
$(TARGET)_OUTPUT   := $($(TARGET)_NAME)-$($(TARGET)_VERSION).zip
$(TARGET)_SCRIPT   := contrib/gitian-descriptors/deps-win32.yml
$(TARGET)_PATCH    := contrib/vagrant/$(HOST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(TARGET).patch
$(TARGET)_PKGS     := db libpng miniupnpc openssl qrencode zlib
$(TARGET)_DEPS     :=
$(TARGET)_FILES    := $($(TARGET)_SCRIPT) \
                      $($(TARGET)_PATCH)
$(TARGET)_FILES    += $(foreach PACKAGE, \
                                $($(TARGET)_PKGS), \
                                contrib/vagrant/cache/$($(PACKAGE)_FILE))
$(TARGET)_FILES    := $(sort $($(TARGET)_FILES))
$(TARGET)_MANIFEST := $(call BUILD_MANIFEST,$($(TARGET)_FILES))
TARGETS            += $(TARGET)

TARGET             := bitcoin-linux
$(TARGET)_NAME     := bitcoin
$(TARGET)_VERSION  := 0.6.99
$(TARGET)_OUTPUT   := $(TARGET)-$($(TARGET)_VERSION)-gitian.zip
$(TARGET)_SCRIPT   := contrib/gitian-descriptors/gitian.yml
$(TARGET)_PATCH    := contrib/vagrant/$(HOST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(TARGET).patch
$(TARGET)_PKGS     := miniupnpc qrencode
$(TARGET)_DEPS     :=
$(TARGET)_FILES    := $($(TARGET)_SCRIPT) \
                      $($(TARGET)_PATCH) \
                      contrib/vagrant/cache/src-local.tar.xz
$(TARGET)_FILES    += $(foreach PACKAGE, \
                                $($(TARGET)_PKGS), \
                                contrib/vagrant/cache/$($(PACKAGE)_FILE))
$(TARGET)_FILES    := $(sort $($(TARGET)_FILES))
$(TARGET)_MANIFEST := $(call BUILD_MANIFEST,$($(TARGET)_FILES))
TARGETS            += $(TARGET)

TARGET             := bitcoin-win32
$(TARGET)_NAME     := bitcoin
$(TARGET)_VERSION  := 0.6.99
$(TARGET)_OUTPUT   := $(TARGET)-$($(TARGET)_VERSION)-gitian.zip
$(TARGET)_SCRIPT   := contrib/gitian-descriptors/gitian-win32.yml
$(TARGET)_PATCH    := contrib/vagrant/$(HOST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(TARGET).patch
$(TARGET)_PKGS     :=
$(TARGET)_DEPS     := boost-win32 qt-win32 bitcoin-deps-win32
$(TARGET)_FILES    := $($(TARGET)_SCRIPT) \
                      $($(TARGET)_PATCH) \
                      contrib/vagrant/cache/src-local.tar.xz
$(TARGET)_FILES    += $(foreach PACKAGE, \
                                $($(TARGET)_PKGS), \
                                contrib/vagrant/cache/$($(PACKAGE)_FILE))
$(TARGET)_FILES    := $(sort $($(TARGET)_FILES))
$(TARGET)_MANIFEST := $(call BUILD_MANIFEST,$($(TARGET)_FILES))
TARGETS            += $(TARGET)

TARGET             := boost-win32
$(TARGET)_NAME     := boost
$(TARGET)_VERSION  := 1.49.0
$(TARGET)_OUTPUT   := $(TARGET)-$($(TARGET)_VERSION)-gitian2.zip
$(TARGET)_SCRIPT   := contrib/gitian-descriptors/$(TARGET).yml
$(TARGET)_PATCH    := contrib/vagrant/$(HOST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(TARGET).patch
$(TARGET)_PKGS     := boost-win32
$(TARGET)_DEPS     :=
$(TARGET)_FILES    := $($(TARGET)_SCRIPT) \
                      $($(TARGET)_PATCH)
$(TARGET)_FILES    += $(foreach PACKAGE, \
                                $($(TARGET)_PKGS), \
                                contrib/vagrant/cache/$($(PACKAGE)_FILE))
$(TARGET)_FILES    := $(sort $($(TARGET)_FILES))
$(TARGET)_MANIFEST := $(call BUILD_MANIFEST,$($(TARGET)_FILES))
TARGETS            += $(TARGET)

TARGET             := qt-win32
$(TARGET)_NAME     := qt
$(TARGET)_VERSION  := 4.7.4
$(TARGET)_OUTPUT   := $(TARGET)-$($(TARGET)_VERSION)-gitian.zip
$(TARGET)_SCRIPT   := contrib/gitian-descriptors/$(TARGET).yml
$(TARGET)_PATCH    := contrib/vagrant/$(HOST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(TARGET).patch
$(TARGET)_PKGS     := qt
$(TARGET)_DEPS     :=
$(TARGET)_FILES    := $($(TARGET)_SCRIPT) \
                      $($(TARGET)_PATCH)
$(TARGET)_FILES    += $(foreach PACKAGE, \
                                $($(TARGET)_PKGS), \
                                contrib/vagrant/cache/$($(PACKAGE)_FILE))
$(TARGET)_FILES    := $(sort $($(TARGET)_FILES))
$(TARGET)_MANIFEST := $(call BUILD_MANIFEST,$($(TARGET)_FILES))
TARGETS            += $(TARGET)

DEPLOY_SOURCE_BUNDLE = \
	vagrant ssh --command "rm -rf ~/'$(1)' || true"; \
	vagrant ssh --command "mkdir -p ~/'$(1)'"; \
	vagrant ssh --command "cd ~/'$(1)' && \
	    tar -xf $(GUEST_CACHE_DIR)/src-local.tar.xz"

define TARGET_RULE
.PHONY: $(1)
$(1): $(HOST_OUTPUT_DIR)/$($(1)_OUTPUT)
$(HOST_OUTPUT_DIR)/$($(1)_OUTPUT): \
	.stamp-gitian-guest-vm.manifest \
	$(HOST_CACHE_DIR)/src-local.manifest \
	$(foreach TARGET,$($(1)_DEPS),$(HOST_OUTPUT_DIR)/$($(TARGET)_OUTPUT)) \
	$(foreach PACKAGE,$($(1)_PKGS),$(HOST_CACHE_DIR)/$($(PACKAGE)_FILE))
	
	mkdir -p $(HOST_OUTPUT_DIR)
	
	$(call VAGRANT_UP)
	
	$(call DEPLOY_SOURCE_BUNDLE,bitcoin)
	vagrant ssh --command "cd ~/bitcoin && \
	    if [ -f $(GUEST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(1).patch ]; then \
	        patch -p1 < $(GUEST_PATCH_DIR)/bitcoin-contrib-gitian-descriptors-$(1).patch || exit 1; \
	    fi"
	
	echo `$($(TARGET)_MANIFEST)` > .$(TARGET).manifest
	
	if [ ! -f $(HOST_CACHE_DIR)/$(TARGET)/`cat .$(TARGET).manifest`/$($(TARGET)_OUTPUT) ]; then \
	    vagrant ssh --command "cd ~/gitian-builder && \
	        USE_LXC=1 ./bin/gbuild ../bitcoin/$($(1)_SCRIPT)" || exit 1; \
	    vagrant ssh --command "cd ~/gitian-builder/build/out && \
	        if [ ! -f $($(TARGET)_OUTPUT) ]; then \
	            zip -r $($(TARGET)_OUTPUT) * || exit 1; \
	        fi" || exit 1; \
	    vagrant ssh --command "mkdir -p '$(GUEST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`/" || exit 1; \
	    vagrant ssh --command "cd ~/gitian-builder && \
	        mv build/out/$($(TARGET)_OUTPUT) \
	           '$(GUEST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`/" || exit 1; \
	    vagrant ssh --command "cd ~/gitian-builder && \
	        mv result/$($(TARGET)_NAME)-res.yml \
	           '$(GUEST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$(1)-$($(TARGET)_VERSION)-gitian-res.yml'" || exit 1; \
	    vagrant ssh --command "cd ~/gitian-builder && \
	        mv var/build.log \
	           '$(GUEST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$(1)-$($(TARGET)_VERSION)-gitian-build.log'" || exit 1; \
	    vagrant ssh --command "cd ~/gitian-builder && \
	        mv var/install.log \
	           '$(GUEST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$(1)-$($(TARGET)_VERSION)-gitian-install.log'" || exit 1; \
	fi
	
	cp -a '$(HOST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$($(TARGET)_OUTPUT)' \
	      '$(HOST_CACHE_DIR)/'
	cp -a '$(HOST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$($(TARGET)_OUTPUT)' \
	      '$(HOST_OUTPUT_DIR)/'
	cp -a '$(HOST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$(1)-$($(TARGET)_VERSION)-gitian-res.yml' \
	      '$(HOST_OUTPUT_DIR)/'
	cp -a '$(HOST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$(1)-$($(TARGET)_VERSION)-gitian-build.log' \
	      '$(HOST_OUTPUT_DIR)/'
	cp -a '$(HOST_CACHE_DIR)/$(TARGET)/'`cat .$(TARGET).manifest`'/$(1)-$($(TARGET)_VERSION)-gitian-install.log' \
	      '$(HOST_OUTPUT_DIR)/'
	
	rm -f .$(TARGET).manifest
	
	vagrant ssh --command "rm -rf ~/bitcoin"
	
	-$(call VAGRANT_HALT)

endef
$(foreach TARGET,$(TARGETS),$(eval $(call TARGET_RULE,$(TARGET))))

# ===----------------------------------------------------------------------===
# End of File
# ===----------------------------------------------------------------------===
