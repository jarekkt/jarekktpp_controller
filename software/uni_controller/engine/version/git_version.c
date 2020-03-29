#include "version.h"



const char  git_hash_short[] =
{
  #include "git-commit-hash-short.txt"
};

const char  git_hash_long[] =
{
  #include "git-commit-hash-long.txt"
};


const char version_string[] = ("PRT-"__DATE__" "__TIME__"\r\n");
const char version_string_git[] = ("GIT-"
#include "git-commit-hash-short.txt"
"\r\n");

