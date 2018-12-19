#!/bin/bash
REPO=greatscottgadgets/ubertooth-nightlies
PUBLICATION_BRANCH=gh-pages
FILE_TO_DEPLOY=
set -x
# Checkout the branch
git clone --branch=$PUBLICATION_BRANCH https://${GITHUB_TOKEN}@github.com/$REPO.git publish
cd publish
# Update pages
cp $ARTEFACT_BASE/$BUILD_NAME.tar.xz .
# Commit and push latest version
git add $BUILD_NAME.tar.xz
git config user.name  "Travis"
git config user.email "travis@travis-ci.org"
git commit -m "FIXME: useful information here"
if [ "$?" != "0" ]; then
    echo "Looks like the commit failed"
fi
git push -fq origin $PUBLICATION_BRANCH 2>&1 > /dev/null