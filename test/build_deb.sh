#!/bin/bash

cmake -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX=/usr \
    -DCPACK_GENERATOR=DEB \
    -DCPACK_DEBIAN_PACKAGE_HOMEPAGE="https://wohlsoft.ru" \
    -DCPACK_DEBIAN_PACKAGE_RELEASE=1 \
    -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=amd64 \
    -DCPACK_DEBIAN_PACKAGE_DEPENDS="libqt5core5a, libqt5gui5, libqt5widgets5, libqt5concurrent5, libqt5serialport5, libqwt-qt5-6, libasound2, libpulse0, libfreetype6" \
    -DCPACK_DEBIAN_PACKAGE_DESCRIPTION="OPL3 Bank Editor - A tone bank editor for OPL2/OPL3 chips" \
$1

ninja

cpack .
