#!/bin/bash
REPO=greatscottgadgets/ubertooth-nightlies
PUBLICATION_BRANCH=gh-pages
FILE_TO_DEPLOY=$ARTEFACT_BASE/$BUILD_NAME.tar.xz
set -x
cd $ARTEFACT_BASE
# Checkout the branch
git clone --branch=$PUBLICATION_BRANCH https://${GITHUB_TOKEN}@github.com/$REPO.git publish
cd publish
# Update pages
cp $FILE_TO_DEPLOY .
# Commit and push latest version
git add $FILE_TO_DEPLOY
git config user.name  "Travis"
git config user.email "travis@travis-ci.org"
git commit -m "FIXME: useful information here"
if [ "$?" != "0" ]; then
    echo "Looks like the commit failed"
fi
git push -fq origin $PUBLICATION_BRANCH 2>&1 > /dev/null