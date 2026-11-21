"""Downloads Qt 6.12.0 for MSVC 2022 x64 from the official Qt repository.

aqtinstall cannot read the repository layout of Qt 6.12 yet, so the packages are fetched directly. Every archive is
checked against the SHA-1 that the repository publishes next to it. Usage: get_qt.py <output folder>
"""
import hashlib
import os
import sys
import urllib.request
import xml.etree.ElementTree as ET

import py7zr

REPOSITORY = "https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6120/qt6_6120_msvc2022_64"
# qtbase, qtsvg, qttools (windeployqt) and the other base archives, plus Qt5Compat
PACKAGES = [
    "qt.qt6.6120.win64_msvc2022_64",
    "qt.qt6.6120.addons.qt5compat.win64_msvc2022_64",
]


# archives of the package that the build does not need (documentation, translations, QML)
SKIP = ("qtdoc-", "qttranslations-", "qtdeclarative-")


def fetch(url):
    with urllib.request.urlopen(url) as response:
        return response.read()


def main():
    output = os.path.join(sys.argv[1], "6.12.0", "msvc2022_64")
    cache = os.path.join(sys.argv[1], "_download")
    os.makedirs(output, exist_ok=True)
    os.makedirs(cache, exist_ok=True)

    root = ET.fromstring(fetch(REPOSITORY + "/Updates.xml"))
    found = set()
    for package in root.iter("PackageUpdate"):
        name = package.findtext("Name")
        if name not in PACKAGES:
            continue
        found.add(name)
        version = package.findtext("Version")
        for archive in package.findtext("DownloadableArchives").split(","):
            archive = archive.strip()
            if not archive or archive.startswith(SKIP):
                continue
            url = "%s/%s/%s%s" % (REPOSITORY, name, version, archive)
            target = os.path.join(cache, archive)
            if not os.path.exists(target):
                print("download", archive, flush=True)
                data = fetch(url)
                expected = fetch(url + ".sha1").decode().split()[0]
                if hashlib.sha1(data).hexdigest() != expected:
                    sys.exit("SHA-1 mismatch for " + archive)
                with open(target, "wb") as handle:
                    handle.write(data)
            print("extract", archive, flush=True)
            with py7zr.SevenZipFile(target) as archive_file:
                archive_file.extractall(output)

    missing = set(PACKAGES) - found
    if missing:
        sys.exit("packages not found in the repository: " + ", ".join(sorted(missing)))
    print("Qt is ready in", output)


main()
