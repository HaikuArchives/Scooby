#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <StorageDefs.h>
#include "scandir.h"
#include <errno.h>
#include <stdio.h>

/* Initial guess at directory size. */ 
#define INITIAL_SIZE    4096

char gName[B_FILE_NAME_LENGTH];

int
scandir(const char *name, struct dirent ***list, int (*selector)(struct dirent *), int (*sorter)(const void *, const void *))
{
    struct dirent        **names; 
    struct dirent        *entp; 
    DIR          *dirp; 
    int           i; 
    int           size; 
    int		dirent_size = sizeof(struct dirent);

    /* Get initial list space and open directory. */ 
    size = INITIAL_SIZE; 
    names = (struct dirent **)malloc(size * sizeof names[0]); 
    if (names == NULL) 
        return -1; 
    dirp = opendir(name);
    strcpy(gName,name); 
    if (dirp == NULL) 
        return -1; 
	i = 0;
    /* Read entries in the directory. */ 
   while ((entp = readdir(dirp)) != NULL ) 
        if (selector == NULL || (*selector)(entp)) { 
            /* User wants them all, or he wants this one. */ 
            if (++i >= size) { 
                size <<= 1; 
                names = (struct dirent **) 
                    realloc((char *)names, size * sizeof names[0]); 
                if (names == NULL) { 
                    closedir(dirp); 
                    return -1; 
                } 
            } 

            /* Copy the entry. */ 
            names[i - 1] = (struct dirent *)malloc(sizeof(struct dirent) 
                                                + entp->d_reclen+1); 
            if (names[i - 1] == NULL) { 
                closedir(dirp); 
                return -1; 
            }
            memcpy(names[i-1],entp,dirent_size);
            memcpy(names[i - 1]->d_name, entp->d_name,entp->d_reclen); 
        	names[i-1]->d_name[entp->d_reclen] = '\0';
        } 

    /* Close things off. */ 
    names[i] = NULL; 
    *list = names; 
    closedir(dirp); 

    /* Sort? */ 
    if (i && sorter) 
        qsort((char *)names, i, sizeof names[0], sorter); 

    return i;
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
int
alphasort(const void *d1, const void *d2)
{
	return(strcmp((*(struct dirent **)d1)->d_name,
	    (*(struct dirent **)d2)->d_name));
}

int
folderselector(struct dirent *dent)
{
	struct stat st;
	char name[B_FILE_NAME_LENGTH];
	
	if(strcmp(dent->d_name,".") == 0||strcmp(dent->d_name,"..") == 0)
		return 0;
	sprintf(name,"%s/%s",gName,dent->d_name);
	stat(name,&st);
	
	if(S_ISLNK(st.st_mode))
		return 0;
	if(S_ISDIR(st.st_mode))
		return 1;
	return 0;
}