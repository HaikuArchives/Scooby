#ifndef __SCANDIR_H__
#define __SCANDIR_H__

#ifdef  __cplusplus
extern "C" {
#endif

//int scandir(const char *dirname, struct dirent ***namelist, int (*select)(struct dirent *), int (*dcomp)(const void *, const void *));
//int alphasort(const void *d1, const void *d2);
int folderselector(const struct dirent *dent);
#ifdef  __cplusplus
}
#endif
#endif
