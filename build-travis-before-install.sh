#!/bin/bash
set -eu -o pipefail
[[ "${TRAVIS_OS_NAME:-}" = linux ]] || exit 0
[[ "${COVERITY_SCAN_BRANCH:-}" = 1 ]] || exit 0
echo -n | openssl s_client -connect https://scan.coverity.com:443 | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | sudo tee -a /etc/ssl/certs/ca-
