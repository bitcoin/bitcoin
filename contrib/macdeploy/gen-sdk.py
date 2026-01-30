#!/usr/bin/env python3
import argparse
import plistlib
import pathlib
import tarfile
import os
import contextlib

@contextlib.contextmanager
def cd(path):
    """Context manager that restores PWD even if an exception was raised."""
    old_pwd = os.getcwd()
    os.chdir(str(path))
    try:
        yield
    finally:
        os.chdir(old_pwd)

def run():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument('xcode_app', metavar='XCODEAPP', type=pathlib.Path)
    parser.add_argument("-o", metavar='OUTSDKTAR', dest='out_sdkt', type=pathlib.Path, required=False)

    args = parser.parse_args()

    xcode_app = args.xcode_app.resolve()
    assert xcode_app.is_dir(), "The supplied Xcode.app path '{}' either does not exist or is not a directory".format(xcode_app)

    xcode_app_plist = xcode_app.joinpath("Contents/version.plist")
    with xcode_app_plist.open('rb') as fp:
        pl = plistlib.load(fp)
        xcode_version = pl['CFBundleShortVersionString']
        xcode_build_id = pl['ProductBuildVersion']
    print("Found Xcode (version: {xcode_version}, build id: {xcode_build_id})".format(xcode_version=xcode_version, xcode_build_id=xcode_build_id))

    sdk_dir = xcode_app.joinpath("Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
    sdk_plist = sdk_dir.joinpath("System/Library/CoreServices/SystemVersion.plist")
    with sdk_plist.open('rb') as fp:
        pl = plistlib.load(fp)
        sdk_version = pl['ProductVersion']
        sdk_build_id = pl['ProductBuildVersion']
    print("Found MacOSX SDK (version: {sdk_version}, build id: {sdk_build_id})".format(sdk_version=sdk_version, sdk_build_id=sdk_build_id))

    out_name = "Xcode-{xcode_version}-{xcode_build_id}-extracted-SDK-with-libcxx-headers".format(xcode_version=xcode_version, xcode_build_id=xcode_build_id)

    out_sdkt_path = args.out_sdkt or pathlib.Path("./{}.tar".format(out_name))

    def tarfp_add_with_base_change(tarfp, dir_to_add, alt_base_dir):
        """Add all files in dir_to_add to tarfp, but prepend alt_base_dir to the files'
        names

        e.g. if the only file under /root/bazdir is /root/bazdir/qux, invoking:

            tarfp_add_with_base_change(tarfp, "foo/bar", "/root/bazdir")

        would result in the following members being added to tarfp:

            foo/bar/             -> corresponding to /root/bazdir
            foo/bar/qux          -> corresponding to /root/bazdir/qux

        """
        def change_tarinfo_base(tarinfo):
            if tarinfo.name and tarinfo.name.endswith((".swiftmodule", ".modulemap")):
                return None
            if tarinfo.name and tarinfo.name.startswith("./"):
                tarinfo.name = str(pathlib.Path(alt_base_dir, tarinfo.name))
            if tarinfo.linkname and tarinfo.linkname.startswith("./"):
                tarinfo.linkname = str(pathlib.Path(alt_base_dir, tarinfo.linkname))
            # make metadata deterministic
            tarinfo.mtime = 0
            tarinfo.uid, tarinfo.uname =  0, ''
            tarinfo.gid, tarinfo.gname = 0, ''
            # don't use isdir() as there are also executable files present
            tarinfo.mode = 0o0755 if tarinfo.mode & 0o0100 else 0o0644
            return tarinfo
        with cd(dir_to_add):
            # recursion already adds entries in sorted order
            tarfp.add("./usr/include", recursive=True, filter=change_tarinfo_base)
            tarfp.add("./usr/lib", recursive=True, filter=change_tarinfo_base)
            tarfp.add("./System/Library/Frameworks", recursive=True, filter=change_tarinfo_base)

    print("Creating output .tar file...")
    with out_sdkt_path.open("wb") as fp:
        with tarfile.open(mode="w", fileobj=fp, format=tarfile.PAX_FORMAT) as tarfp:
            print("Adding MacOSX SDK {} files...".format(sdk_version))
            tarfp_add_with_base_change(tarfp, sdk_dir, out_name)
    print("Done! Find the resulting tarball at:")
    print(out_sdkt_path.resolve())

if __name__ == '__main__':
    run()
