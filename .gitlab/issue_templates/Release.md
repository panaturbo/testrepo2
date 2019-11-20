## Release Schedule

**Tagging Deadline:**

**ASN Deadline:**

**Public Release:**

## Release Checklist

## 2 Working Days Before the Tagging Deadline

 - [ ] ***(QA)*** Check whether all issues assigned to the release milestone are resolved[^1].
 - [ ] ***(QA)*** Ensure that there are no outstanding merge requests in the private repository[^1] (Subscription Edition only).

## Before the Tagging Deadline

 - [ ] ***(QA)*** Inform Support/Marketing of impending release (and give estimated release dates).
 - [ ] ***(QA)*** Check Perflab to ensure there has been no unexplained drop in performance for the versions being released.
 - [ ] ***(SwEng)*** Update API files for libraries with new version information.
 - [ ] ***(SwEng)*** Change software version and library versions in `configure.ac` (new major release only).
 - [ ] ***(SwEng)*** Rebuild `configure` using Autoconf on `docs.isc.org`.
 - [ ] ***(SwEng)*** Update `CHANGES`.
 - [ ] ***(SwEng)*** Update `CHANGES.SE` (Subscription Edition only).
 - [ ] ***(SwEng)*** Update `README.md`.
 - [ ] ***(SwEng)*** Update `version`.
 - [ ] ***(SwEng)*** Build documentation on `docs.isc.org`.
 - [ ] ***(QA)*** Check that all the above steps were performed correctly.
 - [ ] ***(QA)*** Check that the contents of release notes match the merge requests comprising the releases.
 - [ ] ***(QA)*** Check that the formatting is correct for text, PDF, and HTML versions of release notes.
 - [ ] ***(SwEng)*** Tag the releases[^2].  (Tags may only be pushed to the public repository for releases which are *not* security releases.)
 - [ ] ***(SwEng)*** If this is the first tag for a release (e.g. beta), create a release branch named `release_v9_X_Y` to allow development to continue on the maintenance branch whilst release engineering continues.

## Before the ASN Deadline (for ASN Releases) or the Public Release Date (for Regular Releases)

 - [ ] ***(QA)*** Run the `make release` Jenkins jobs to produce the tarballs and zips.
 - [ ] ***(QA)*** Verify the results of `make release` Jenkins jobs and prepare a QA report for the releases to be published.
 - [ ] ***(QA)*** Request signatures for the tarballs.
 - [ ] ***(Signers)*** Sign the tarballs.
 - [ ] ***(QA)*** Check tarball signatures.
 - [ ] ***(QA)*** Notify Support that the releases are ready for publication.
 - [ ] ***(Support)*** Pre-publish ASN and/or Subscription Edition tarballs so that packages can be built.
 - [ ] ***(QA)*** Build and test ASN and/or Subscription Edition packages.
 - [ ] ***(Support)*** Send out ASNs (if applicable).

## On the Day of Public Release

 - [ ] ***(Support)*** Publish the releases according to the release schedule.
 - [ ] ***(Support)*** Write release email to *bind9-announce*.
 - [ ] ***(Support)*** Write email to *bind9-users* (if a major release).
 - [ ] ***(Support)*** Update tickets in case of waiting support customers.
 - [ ] ***(QA)*** Build and test any outstanding private packages.
 - [ ] ***(QA)*** Build public packages (`*.deb`, RPMs).
 - [ ] ***(QA)*** Inform Marketing of the release.
 - [ ] ***(QA)*** Update the internal [BIND release dates wiki page](https://wiki.isc.org/bin/view/Main/BindReleaseDates) when public announcement has been made.
 - [ ] ***(Marketing)*** Post short note to Twitter.
 - [ ] ***(Marketing)*** Update [Wikipedia entry for BIND](https://en.wikipedia.org/wiki/BIND).
 - [ ] ***(Marketing)*** Write blog article (if a major release).
 - [ ] ***(QA)*** Ensure all new tags are annotated and signed.
 - [ ] ***(SwEng)*** Push tags for the published releases to the public repository.
 - [ ] ***(SwEng)*** Merge the automatically prepared `prep 9.X.Y` commit which updates `version` and documentation on the release branch into the relevant maintenance branch (`v9_X`).

[^1]: If not, use the time remaining until the tagging deadline to ensure all outstanding issues are either resolved or moved to a different milestone.

[^2]: Preferred command line: `git tag -u <DEVELOPER_KEYID> -a -s -m "BIND 9.X.Y[alphatag]" v9_X_Y[alphatag]`, where `[alphatag]` is an optional string such as `b1`, `rc1`, etc.
