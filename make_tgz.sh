#!/bin/bash

echo Making update tgz file in tgz directory...
mkdir -p tgz
tar czf tgz/update_RackmonIoc.tgz `find RackmonIoc/* lib/ bin/ -name tgz -prune -o -name \*.tgz -prune  -o -name \*linux-x86\* -prune -o -name O.\* -prune -o -name src -prune -o -print`
echo Making deployment tgz file in tgz directory...
tar czf tgz/deploy_RackmonIoc.tgz `find RackmonIoc/* lib/ bin/ */lib */bin */db */dbd  -name tgz -prune -o -name \*.tgz -prune -o -name \*linux-x86\* -prune -o -name O.\* -prune -o -name src -prune -o -print`
