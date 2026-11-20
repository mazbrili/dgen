#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
ROOT=$(pwd)
. ./haiku-app.env

ARCH=$(getarch)
OUT="$ROOT/${APP_CMD}-${VERSION}-${REVISION}-${ARCH}.hpkg"

for tool in getarch mimeset package rc xres; do
	command -v "$tool" >/dev/null 2>&1 || {
		echo "build-hpkg: '$tool' not found (pkgman install haiku_devel)" >&2
		exit 1
	}
done

sh ./autogen.sh
./configure --enable-haiku --disable-asm
make clean >/dev/null
make LDFLAGS="${LDFLAGS:-} -s"

PKG="$ROOT/obj/hpkg"
rm -rf "$PKG"
mkdir -p "$PKG/apps" "$PKG/bin" "$PKG/data/deskbar/menu/Applications" \
	"$PKG/documentation/packages/$APP_CMD"
cp "$APP_NAME" "$PKG/apps/$APP_NAME"
mimeset -f "$PKG/apps/$APP_NAME"
ln -s "../apps/$APP_NAME" "$PKG/bin/$APP_CMD"
ln -s "../../../../apps/$APP_NAME" \
	"$PKG/data/deskbar/menu/Applications/$DESKBAR_NAME"
cp COPYING README "$PKG/documentation/packages/$APP_CMD/"

DESC=$(sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e '2,$s/^/\t/' "$DESCRIPTION_FILE")
cat > "$PKG/.PackageInfo" <<EOF
name			$APP_CMD
version			$VERSION-$REVISION
architecture	$ARCH
summary			"$SUMMARY"
description		"$DESC"
packager		"$PACKAGER"
vendor			"$APP_NAME"
copyrights		{
	"$COPYRIGHT"
}
licenses		{
	"$LICENSE_NAME"
}
urls			{
	"$HOMEPAGE"
}
provides		{
	$APP_CMD = $VERSION
	app:$APP_NAME = $VERSION
	cmd:$APP_CMD
}
requires		{
	haiku >= r1~beta6
}
EOF

rm -f "$OUT"
package create -C "$PKG" "$OUT"
package list -i "$OUT" >/dev/null
echo "Built $OUT"
