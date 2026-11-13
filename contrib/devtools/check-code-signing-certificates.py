#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

import http.client
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request


# Format: /path/to/cert, <expiry in days>
CERTIFICATES = [
    ("contrib/windeploy/win-codesign.cert", 90),
]
# Exit 0 if checks pass, 2 if any check fails, and 3 if any certificate needs
# replacement. Reserve 1 for unhandled Python errors.


def output(*command: str) -> str:
    return subprocess.check_output(command, text=True).rstrip("\n")


def check_certificate(cert: str, days: int) -> tuple[str, bool]:
    x509 = ["openssl", "x509", "-in", cert, "-noout"]
    extensions = output(*x509, "-ext", "basicConstraints,extendedKeyUsage")
    # Require the first certificate to be a non-CA code signing certificate.
    if "CA:FALSE" not in extensions or "Code Signing" not in extensions:
        raise ValueError(
            f"The first certificate in {cert} must be a non-CA code signing certificate."
        )

    expiry = output(*x509, "-enddate").removeprefix("notAfter=")
    warning_check = subprocess.run(
        [*x509, "-checkend", str(days * 86400)], stdout=subprocess.DEVNULL
    )
    if warning_check.returncode == 0:
        distribution_points = output(*x509, "-ext", "crlDistributionPoints")
        crl_url = re.search(r"URI:(https?://\S+)", distribution_points)
        if crl_url is None:
            raise ValueError(f"No HTTP CRL distribution point found in {cert}.")
        # Download separately to avoid older OpenSSL versions' 100 KiB limit.
        with (
            urllib.request.urlopen(crl_url[1], timeout=30) as response,
            tempfile.NamedTemporaryFile() as crl,
        ):
            shutil.copyfileobj(response, crl)
            crl.flush()
            verification = subprocess.run(
                [
                    "openssl",
                    "verify",
                    "-crl_check",
                    "-CRLfile",
                    crl.name,
                    "-untrusted",
                    cert,
                    cert,
                ],
                capture_output=True,
                text=True,
                timeout=30,
            )
        if verification.returncode == 0:
            return (
                f"{cert}: CRL check passed; expires {expiry} (at least {days} days away).",
                False,
            )
        # Only alert on leaf revocation with no other verification errors,
        # such as a stale CRL that also contains the certificate's serial.
        errors = re.findall(
            r"^error (\d+) at (\d+) depth lookup:", verification.stderr, re.MULTILINE
        )
        if errors != [("23", "0")]:
            raise ValueError(
                f"Revocation check failed for {cert}:\n{verification.stderr}"
            )
        reason = "has been revoked"
    else:
        # Preserve renewal alerts even when the CRL service is unavailable.
        reason = "is expiring or has expired"

    details = output(*x509, "-subject", "-issuer", "-dates", "-sha256", "-fingerprint")
    # Keep the report stable when an expiring certificate becomes expired,
    # so CI can recognize an already acknowledged alert.
    return (
        f"{cert}: {reason}.\nWarning window: {days} days.\n{details}",
        True,
    )


def main() -> int:
    reports = []
    warnings = []
    failed = False
    for cert, days in CERTIFICATES:
        try:
            report, warning = check_certificate(cert, days)
            reports.append(report)
            if warning:
                warnings.append(report)
        except (
            http.client.HTTPException,
            OSError,
            subprocess.SubprocessError,
            ValueError,
        ) as error:
            print(error, file=sys.stderr)
            failed = True
    print("\n\n".join(warnings or reports))
    if failed:
        return 2
    return 3 if warnings else 0


if __name__ == "__main__":
    sys.exit(main())
