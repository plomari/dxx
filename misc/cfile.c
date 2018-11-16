#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "cfile.h"

// PATH_MAX tends to be bogus - don't use it.
#define PATH_LEN 4096

static char write_path[PATH_LEN];
static char data_path[PATH_LEN];

static char *search_paths[] = {write_path, data_path, NULL};

struct hogfile {
	char name[14];
	int offset, length;
};

#define MAX_HOGFILES 300

struct hogmount {
	char filename[PATH_LEN];
	CFILE *file;
	int64_t file_pos;
	struct hogfile entries[MAX_HOGFILES];
	size_t num_entries;
};

#define HOG_SLOT_COUNT 7

static struct hogmount hogs_alloc[HOG_SLOT_COUNT];
static struct hogmount *hogs[HOG_SLOT_COUNT];

typedef struct CFILE {
	bool			owns_file;			// call fclose(file) when destroying CFILE
	bool			eof_flag;
	bool			err_flag;
	FILE 			*file;				// underlying file
	int64_t			*file_pos;			// pointer to cached ftell() of file
	int64_t			sub_offset;			// first byte in file for hog entry
	int64_t			sub_size;			// number of bytes in hog entry
	int64_t			sub_pos;			// our own position within the hog entry
	int64_t			local_file_pos; 	// use file_pos instead
} CFILE;

// Set up basic search paths.
void cfile_init_paths(int argc, char *argv[])
{
	char *home = getenv("HOME");
	if (!home || !home[0]) {
		fprintf(stderr, "$HOME not set.\n");
		exit(1);
	}

	int r = snprintf(write_path, sizeof(write_path), "%s/.d2x-rebirth/", home);
	if (r >= sizeof(write_path)) {
		fprintf(stderr, "$HOME path too long.\n");
		exit(1);
	}

	const char *data = SHAREPATH;
	if (GameArg.SysHogDir)
		data = GameArg.SysHogDir;

	r = snprintf(data_path, sizeof(data_path), "%s", data);
	if (r >= sizeof(data_path)) {
		fprintf(stderr, "Data path too long.\n");
		exit(1);
	}
}

int cfclose(CFILE *cfile)
{
	if (!cfile)
		return 0;

	int r;
	if (cfile->owns_file) {
		r = fclose(cfile->file);
	} else {
		r = fflush(cfile->file);
	}

	free(cfile);

	return r;
}

// For the sake of the physfs wrapper, just kill me.
int cfclose_inv( CFILE * cfile )
{
	return !cfclose(cfile);
}

// If sub_size < 0, try to determine it automatically.
// Sub ranges are generally ignored for write accesses.
static CFILE *open_cfile(FILE *f, bool owns_f, int64_t *pos,
						 int64_t sub_offset, int64_t sub_size)
{
	if (!f)
		return NULL;

	CFILE *cf = malloc(sizeof(*cf));
	if (!cf) {
		if (owns_f && f)
			fclose(f);
		return NULL;
	}

	*cf = (CFILE){
		.file 		= f,
		.owns_file 	= owns_f,
		.sub_offset	= sub_offset,
		.sub_size 	= sub_size,
		.file_pos	= pos ? pos : &cf->local_file_pos,
	};

	if (cf->sub_size < 0) {
		if (fseeko(cf->file, 0, SEEK_END)) {
			cf->sub_size = 0;
		} else {
			cf->sub_size = ftello(cf->file);
		}
	}

	if (cfseek(cf, 0, SEEK_SET)) {
		fprintf(stderr, "initial seek failed\n");
		cf->err_flag = true;
	}

	return cf;
}

static bool lookup_file(const char *filename, char *res, size_t res_size)
{
	if (res_size)
		res[0] = '\0';

	// We need to enumerate all directory entries because it could be a
	// case-insensitive match.

	// Separate into path and filename components. (Breaks trying to open dirs.)
	size_t base_len = 0;
	const char *basepath, *entryname;
	char *sep = strrchr(filename, '/');
	if (sep) {
		entryname = sep + 1;
		base_len = sep - filename + 1;
		if (base_len >= res_size)
			return false;
		memcpy(res, filename, base_len);
		res[base_len] = '\0';
		basepath = res;
	} else {
		entryname = filename;
		basepath = ".";
	}

	// (If we find nothing, fallback to input casing. Using the fallback is
	// required for write accesses that create files and such.)
	const char *resentryname = entryname;

	DIR *dir = opendir(basepath);
	if (!dir)
		return false;

	while (1) {
		struct dirent *ent = readdir(dir);
		if (!ent)
			break;
		const char *name = ent->d_name;

		if (stricmp(name, entryname) == 0) {
			resentryname = name;
			break;
		}
	}

	size_t resentryname_len = strlen(resentryname);
	bool ok;
	if (resentryname_len + 1 <= res_size - base_len) {
		ok = true;
		memcpy(res + base_len, resentryname, resentryname_len + 1);
	} else {
		ok = false;
	}

	closedir(dir);

	return ok;
}

static bool join_paths(const char *a, const char *b, char *res, size_t res_size)
{
	if (b[0] == '/') {
		size_t len = strlen(b);
		if (len + 1 >= res_size)
			return false;
		memcpy(res, b, len + 1);
	} else {
		size_t len_a = strlen(a);
		size_t len_b = strlen(b);
		if (len_a + 1 + len_b + 1 >= res_size)
			return false;
		memcpy(res, a, len_a);
		if (len_a && a[len_a - 1] != '/')
			res[len_a++] = '/';
		memcpy(res + len_a, b, len_b + 1);
	}
	return true;
}

static CFILE *cfopen_in_dir(const char *path, const char *filename, const char *mode)
{
	char fullpath[PATH_LEN];
	char finalpath[PATH_LEN];
	if (!join_paths(path, filename, fullpath, sizeof(fullpath)))
		return NULL;
	if (!lookup_file(fullpath, finalpath, sizeof(finalpath)))
		return NULL;

	return open_cfile(fopen(finalpath, mode), true, NULL, 0, -1);
}

CFILE *cfopen(const char *filename, const char *mode)
{
	// Write accesses are redirected to user path.
	// Never try to write to other locations.
	if (strchr(mode, 'w'))
		return cfopen_in_dir(write_path, filename, mode);

	for (size_t n = 0; n < ARRAY_ELEMS(hogs); n++) {
		struct hogmount *hog = hogs[n];
		for (size_t i = 0; hog && i < hog->num_entries; i++) {
			struct hogfile *f = &hog->entries[i];
			if (stricmp(f->name, filename) == 0)
				return open_cfile(hog->file->file, false, &hog->file_pos, f->offset, f->length);
		}
	}

	for (size_t n = 0; search_paths[n]; n++) {
		CFILE *res = cfopen_in_dir(search_paths[n], filename, mode);
		if (res)
			return res;
	}

	return NULL;
}

int cfexist(const char *fname)
{
	CFILE *f = cfopen(fname, "rb");
	int exists = !!f;
	cfclose(f);
	return exists;
}

int64_t cfile_size(char *hogname)
{
	CFILE *f = cfopen(hogname, "rb");
	int64_t res = f ? cfilelength(f) : -1;
	cfclose(f);
	return res;
}

size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp )
{
	if (*fp->file_pos != fp->sub_offset + fp->sub_pos) {
		if (fseeko(fp->file, fp->sub_offset + fp->sub_pos, SEEK_SET)) {
			fp->err_flag = true;
			return 0;
		}
	}

	int64_t left = fp->sub_size - fp->sub_pos;

	if (elsize * nelem > left) {
		nelem = left / elsize;
		fp->eof_flag = true;
	}

	size_t r = fread(buf, elsize, nelem, fp->file);

	size_t bytes = r * elsize;
	fp->sub_pos += bytes;
	*fp->file_pos += bytes;

	return r;
}

int64_t cftell( CFILE * fp )
{
	return fp->sub_pos;
}

int64_t cfilelength( CFILE *fp )
{
	return fp->sub_size;
}

int cfile_ferror(CFILE *fp)
{
	return fp->err_flag;
}

int cfile_feof(CFILE *fp)
{
	return fp->eof_flag;
}

int cfseek(CFILE *fp, int64_t offset, int where)
{
	switch (where) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += fp->sub_pos;
		break;
	case SEEK_END:
		offset += fp->sub_size;
		break;
	default:
		abort();
	}

	if (offset < 0 || offset > fp->sub_size)
		return -1;

	int r = fseeko(fp->file, fp->sub_offset + offset, SEEK_SET);
	*fp->file_pos = ftello(fp->file);
	fp->sub_pos = *fp->file_pos - fp->sub_offset;
	return r;
}


int cfile_hog_add(char *hogname, int add_to_end)
{
	struct hogmount *hog = NULL;

	for (size_t n = 0; n < ARRAY_ELEMS(hogs_alloc); n++) {
		struct hogmount *cur = &hogs_alloc[n];
		if (!cur->file) {
			hog = cur;
			break;
		}
	}

	if (!hog) {
		fprintf(stderr, "no free HOG slot\n");
		return 0;
	}

	if (snprintf(hog->filename, sizeof(hog->filename), "%s", hogname)
			>= sizeof(hog->filename))
	{
		return 0;
	}

	hog->file = cfopen(hogname, "rb");
	if (!hog->file)
		return 0;

	if (hog->file->sub_offset != 0) {
		Error("Recursive HOG file\n");
		goto err;
	}

	CFILE *fp = hog->file;

	bool is_dmvl = false;
	int64_t offset = 0;
	int32_t n_files = INT_MAX;

	char id[5] = {0};
	cfread(id, 3, 1, fp);
	if (strcmp(id, "DHF") != 0 && strcmp(id, "D2X") != 0) {
		cfread(id + 3, 1, 1, fp);
		if (strcmp(id, "DMVL") != 0)
			goto err;
		is_dmvl = true;
		cfread(&n_files, sizeof(n_files), 1, fp);
		offset = 4+4+n_files*(13+4); //id + nfiles + nfiles * (filename + size)
	}


	while(1)	{
		if (hog->num_entries >= n_files)
			break;

		if (hog->num_entries >= MAX_HOGFILES) {
			Error("HOGFILE is limited to %d files.\n", MAX_HOGFILES);
			goto err;
		}

		struct hogfile *entry = &hog->entries[hog->num_entries];

		size_t i = cfread(entry->name, 13, 1, fp);
		if (i != 1)
			break; // archive end
		int32_t len;
		i = cfread(&len, 4, 1, fp);
		if (i != 1)
			goto err;
		// D2X-XL crap
		if (len < 0) {
			len = -len;
			char long_name[256 + 1] = {0};
			if (cfread(long_name, sizeof(long_name) - 1, 1, fp) != 1)
				goto err;
			size_t len = strlen(long_name);
			if (len >= sizeof(entry->name)) {
				Error("HOGFILE has long names\n");
				goto err;
			}
			memcpy(entry->name, long_name, len + 1);
		}
		entry->length = len;
		entry->offset = is_dmvl ? offset : cftell(fp);
		hog->num_entries++;
		// Skip over
		if (!is_dmvl && cfseek(fp, len, SEEK_CUR))
			goto err;
		offset += len;
	}

	if (add_to_end) {
		for (size_t n = 0; n < ARRAY_ELEMS(hogs); n++) {
			if (!hogs[n]) {
				hogs[n] = hog;
				break;
			}
		}
	} else {
		for (size_t n = ARRAY_ELEMS(hogs) - 1; n >= 1; n--)
			hogs[n] = hogs[n - 1];
		hogs[0] = hog;
	}

	return 1;

err:
	cfclose(hog->file);
	hog->file = NULL;
	hog->num_entries = 0;
	return 0;
}

int cfile_hog_remove(char *hogname)
{
	for (size_t n = 0; n < ARRAY_ELEMS(hogs); n++) {
		struct hogmount *hog = hogs[n];

		if (hog && stricmp(hog->filename, hogname) == 0) {
			cfclose(hog->file);
			hog->file = NULL;
			hog->num_entries = 0;
			for (size_t i = n; i < ARRAY_ELEMS(hogs) - 1; i++)
				hogs[i] = hogs[i + 1];
			hogs[ARRAY_ELEMS(hogs) - 1] = NULL;
			return 1;
		}
	}

	return 0;
}

size_t cfile_write(CFILE *file, const void *ptr, size_t elsize, size_t nelem)
{
	size_t res = fwrite(ptr, elsize, nelem, file->file);
	file->err_flag |= ferror(file->file);
	file->sub_pos += res * elsize;
	return res;
}

// Write exactly field_len bytes. If str is longer than that, panic. If str is
// exactly as long as that, don#t write a terminating \0. Otherwise, fill with
// \0.
// Returns success.
bool cfile_write_fixed_str(CFILE *file, size_t field_len, const char *str)
{
	size_t len = strlen(str);

	Assert(len <= field_len);
	if (!(len <= field_len))
		return false;

	if (cfile_write(file, str, 1, len) < len)
		return false;

	while (len < field_len) {
		if (cfile_write(file, &(char){0}, 1, 1) < 1)
			return false;
		len++;
	}

	return true;
}

// Read a string that's stored as field_len bytes in the file.
// Warning: sizeof(buf) must be >= field_len+1.
// Returns success. buf is always 0 terminated after this call.
bool cfile_read_fixed_str(CFILE *file, size_t field_len, char *buf)
{
	int res = cfread(buf, field_len, 1, file);
	if (res < 1) {
		buf[0] = '\0';
		return false;
	}
	buf[field_len] = '\0'; // redundant, unless str has length field_len
	return true;
}

int cfgetc(CFILE *fp)
{
	unsigned char c;

	if (cfread(&c, 1, 1, fp) != 1)
		return EOF;

	return c;
}

char * cfgets(char *buf, size_t n, CFILE *fp)
{
	size_t i;
	int c;

	for (i = 0; n && i < n - 1; i++)
	{
		do
		{
			c = cfgetc(fp);
			if (c == EOF)
			{
				*buf = 0;

				return NULL;
			}
			if (c == 0 || c == 10)  // Unix line ending
				break;
			if (c == 13)            // Mac or DOS line ending
			{
				int c1;

				c1 = cfgetc(fp);
				if (c1 != EOF)  // The file could end with a Mac line ending
					cfseek(fp, -1, SEEK_CUR);
				if (c1 == 10) // DOS line ending
					continue;
				else            // Mac line ending
					break;
			}
		} while (c == 13);
		if (c == 13)    // because cr-lf is a bad thing on the mac
			c = '\n';   // and anyway -- 0xod is CR on mac, not 0x0a
		if (c == '\n')
			break;
		*buf++ = c;
	}
	*buf = 0;

	return buf;
}

void cfile_gets_0(CFILE *file, char *s, size_t sz)
{
	cfgets(s, sz, file);
}

void cfile_gets_nl(CFILE *file, char *s, size_t sz)
{
	cfgets(s, sz, file);
}

/*
 * read some data types...
 */

int cfile_read_int(CFILE *file)
{
	int32_t i;

	if (!cfread(&i, sizeof(i), 1, file))
	{
		Error("Error reading int in cfile_read_int()");
		exit(1);
	}

	return i;
}

short cfile_read_short(CFILE *file)
{
	int16_t s;

	if (!cfread(&s, sizeof(s), 1, file))
	{
		Error("Error reading short in cfile_read_short()");
		exit(1);
	}

	return s;
}

sbyte cfile_read_byte(CFILE *file)
{
	sbyte b;

	if (!cfread(&b, 1, 1, file))
	{
		Error("Error reading byte in cfile_read_byte()");
		exit(1);
	}

	return b;
}

fix cfile_read_fix(CFILE *file)
{
	int32_t f;  // a fix is defined as a long for Mac OS 9, and MPW can't convert from (long *) to (int *)

	if (!cfread(&f, sizeof(f), 1, file))
	{
		Error("Error reading fix in cfile_read_fix()");
		exit(1);
	}

	return f;
}

fixang cfile_read_fixang(CFILE *file)
{
	int16_t f;

	if (!cfread(&f, sizeof(f), 1, file))
	{
		Error("Error reading fixang in cfile_read_fixang()");
		exit(1);
	}

	return f;
}

void cfile_read_vector(vms_vector *v, CFILE *file)
{
	v->x = cfile_read_fix(file);
	v->y = cfile_read_fix(file);
	v->z = cfile_read_fix(file);
}

void cfile_read_angvec(vms_angvec *v, CFILE *file)
{
	v->p = cfile_read_fixang(file);
	v->b = cfile_read_fixang(file);
	v->h = cfile_read_fixang(file);
}

void cfile_read_matrix(vms_matrix *m, CFILE*file)
{
	cfile_read_vector(&m->rvec,file);
	cfile_read_vector(&m->uvec,file);
	cfile_read_vector(&m->fvec,file);
}

int cfile_rename(char *oldpath, char *newpath)
{
	char old[PATH_LEN], new[PATH_LEN];

	if (!join_paths(write_path, oldpath, old, sizeof(old)) ||
		!join_paths(write_path, newpath, new, sizeof(new)))
		return 0;

	return rename(old, new) == 0;
}

int cfile_mkdir(const char *path)
{
	char dst[PATH_LEN];
	if (!join_paths(write_path, path, dst, sizeof(dst)))
		return -1;
	return mkdir(dst, 0700);
}

int cfile_unlink(const char *path)
{
	char dst[PATH_LEN];
	if (!join_paths(write_path, path, dst, sizeof(dst)))
		return -1;
	return unlink(dst);
}

//Open a 'data' file for reading (a file that would go in the 'Data' folder for the original Mac Descent II)
//Allows backwards compatibility with old Mac directories while retaining PhysicsFS flexibility
CFILE *cfile_open_data_dir_file(const char *filename)
{
	CFILE *fp = cfopen(filename, "rb");

	if (!fp)
	{
		char name[PATH_LEN + 5];

		snprintf(name, sizeof(name), "Data/%s", filename);
		fp = cfopen(name, "rb");
	}

	return fp;
}

// Find files at path that have an extension listed in exts
// The extension list exts must be NULL-terminated, with each ext beginning with a '.'
// If exts is NULL, return all files.
// Returns NULL on OOM.
// Does not return "." and ".." entries.
char **cfile_find_files(char *path, char **exts)
{
	size_t count = 0;
	char **list = malloc(sizeof(char *));
	if (!list)
		goto done;

	for (size_t n = 0; search_paths[n]; n++) {
		// Someone is listing an absolute path. Joining with the search paths
		// would always return the original path; avoid adding files multiple
		// times.
		if (path[0] == '/' && n > 0)
			break;

		char fullpath[PATH_LEN];
		if (!join_paths(search_paths[n], path, fullpath, sizeof(fullpath)))
			continue;

		DIR *dir = opendir(fullpath);
		if (!dir)
			continue;

		while (1) {
			struct dirent *ent = readdir(dir);
			if (!ent)
				break;
			const char *name = ent->d_name;

			if (strcmp(name, "..") == 0 || strcmp(name, ".") == 0)
				continue;

			if (exts) {
				char *ext = strrchr(name, '.');
				bool found = false;
				for (size_t n = 0; exts[n]; n++) {
					if (ext && stricmp(ext, exts[n]) == 0) {
						found = true;
						break;
					}
				}
				if (!found)
					continue;
			}

			//(If multiple search paths find files with the same names, callers
			//can by design not distinguish them.)
			//char filepath[PATH_LEN];
			//if (!join_paths(fullpath, name, filepath, sizeof(filepath)))
			//	continue;
			const char *filepath = name;

			char **new = realloc(list, (count + 2) * sizeof(char *));
			if (!new) {
				cfile_find_files_free(list);
				list = NULL;
				closedir(dir);
				goto done;
			}
			list = new;
			list[count] = strdup(filepath);
			if (!list[count]) {
				cfile_find_files_free(list);
				list = NULL;
				closedir(dir);
				goto done;
			}
			count++;
		}

		closedir(dir);
	}
	list[count] = NULL;

done:
	return list;
}

void cfile_find_files_free(char **files)
{
	for (size_t n = 0; files && files[n]; n++)
		free(files[n]);
	free(files);
}

bool cfile_is_directory(char *path)
{
	char fullpath[PATH_LEN];
	if (!lookup_file(path, fullpath, sizeof(fullpath)))
		return false;
	struct stat st;
	return stat(fullpath, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}
