commits=`git rev-list HEAD --count`
echo "#ifndef SPN_VERISON_H_
#define SPN_VERSION_H_
#define SPN_VERSION $commits
#endif" > src/include/spn_version.h
echo $commits > VERSION
