# Distributed under the terms of the GNU General Public License v2
EAPI=5
LANGS="en ru"

inherit db-use eutils fdo-mime gnome2-utils kde4-functions qt4-r2

DB_VER="4.8"

DESCRIPTION="NovaCoin - a hybrid PoW+PoS energy efficient p2p-cryptocurrency and electronic payment system."
HOMEPAGE="https://novaco.in/"
SRC_URI="https://github.com/${PN}-project/${PN}/archive/nvc-v${PV}.tar.gz -> ${PN}-${PV}.tar.gz"

LICENSE="MIT"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE="+dbus -ipv6 kde +qrcode"

RDEPEND="
	dev-libs/boost[threads(+)]
	dev-libs/openssl:0[-bindist]
	qrcode? (
		media-gfx/qrencode
	)
	sys-libs/db:$(db_ver_to_slot "${DB_VER}")[cxx]
	dev-qt/qtgui:4
	dbus? (
		dev-qt/qtdbus:4
	)
"
DEPEND="${RDEPEND}
	>=app-shells/bash-4.1
	>sys-devel/gcc-4.3.3
	>=dev-libs/openssl-0.9.8g
	>=sys-libs/db-4.8.30
	>=dev-libs/boost-1.37
"

DOCS="doc/translation_process.md"

S="${WORKDIR}/${PN}-nvc-v${PV}"

src_prepare() {
	cd src || die

	local filt= yeslang= nolang=

	for ts in $(ls qt/locale/*.ts)
	do
		x="${ts/*bitcoin_/}"
		x="${x/.ts/}"
		if ! use "linguas_$x"; then
			nolang="$nolang $x"
			rm "$ts"
			filt="$filt\\|$x"
		else
			yeslang="$yeslang $x"
		fi
	done

	filt="bitcoin_\\(${filt:2}\\)\\.\(qm\|ts\)"
	sed "/${filt}/d" -i 'qt/bitcoin.qrc'
	einfo "Languages -- Enabled:$yeslang -- Disabled:$nolang"
}

src_configure() {
	OPTS=()

	use dbus && OPTS+=("USE_DBUS=1")
	use ipv6 || OPTS+=("USE_IPV6=-")

	OPTS+=("BDB_INCLUDE_PATH=$(db_includedir "${DB_VER}")")
	OPTS+=("BDB_LIB_SUFFIX=-${DB_VER}")

	if has_version '>=dev-libs/boost-1.52'; then
		OPTS+=("LIBS+=-lboost_chrono\$\$BOOST_LIB_SUFFIX")
	fi

	eqmake4 ${PN}-qt.pro "${OPTS[@]}"
}

src_install() {
	dobin ${PN}-qt

	insinto /usr/share/pixmaps
	newins "src/qt/res/icons/novacoin-128.png" "${PN}.png"

	make_desktop_entry "${PN}-qt" "Novacoin" "/usr/share/pixmaps/${PN}.png" "Network;P2P;Finance;"

	if use kde; then
		insinto /usr/share/kde4/services
		newins contrib/debian/novacoin-qt.protocol ${PN}.protocol
	fi
}

update_caches() {
	gnome2_icon_cache_update
	fdo-mime_desktop_database_update
	buildsycoca
}

pkg_postinst() {
	update_caches
}

pkg_postrm() {
	update_caches
}