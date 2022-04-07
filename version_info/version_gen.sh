BUILD_TIME=`date "+%Y-%m-%d %H:%M:%S"`
WHO_BUILD=`git config user.name`
BRANCH_NAME=`git branch | grep "*"`
COMMIT_ID=`git rev-parse HEAD`
echo -e "#ifndef _VERSION_H_" > ./version_info/version.h
echo -e "#define _VERSION_H_\n" >> ./version_info/version.h
echo -e "#define MODE \"$MODE\"" >> ./version_info/version.h
echo -e "#define PLATFORM \"$PLATFORM\"" >> ./version_info/version.h
echo -e "#define REDIRECT \"$REDIRECT\"" >> ./version_info/version.h
echo -e "#define BUILD_TIME \"$BUILD_TIME\"" >> ./version_info/version.h
echo -e "#define WHO_BUILD \"$WHO_BUILD\"" >> ./version_info/version.h
echo -e "#define BRANCH_NAME \"$BRANCH_NAME\"" >> ./version_info/version.h
echo -e "#define COMMIT_ID \"$COMMIT_ID\"" >> ./version_info/version.h
echo -e "extern void show_version(void);" >> ./version_info/version.h
echo -e "\n#endif\n" >> ./version_info/version.h
