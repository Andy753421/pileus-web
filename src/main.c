#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <markdown_lib.h>

#include "html.h"

/* HTML helper functions */
void href(const char *url)
{
	const char *path   = getenv("REQUEST_URI");
	const char *script = getenv("SCRIPT_NAME");
	if (path && script) {
		for (; *path; path++, script++)
			if (*path != *script)
				break;
		for (; *path; path++)
			if (*path == '/')
				printf("../");
	}
	printf("%s", url[0] ? url : ".");
}

int str_sort(gconstpointer _a, gconstpointer _b)
{
	const char **a = (const char**)_a;
	const char **b = (const char**)_b;
	return strcmp(*a, *b);
}

gchar **lsdir(gchar *path)
{
	GPtrArray *ents = g_ptr_array_new();
	GDir      *dir  = g_dir_open(path, 0, NULL);
	if (dir) {
		const char *name;
		while ((name = g_dir_read_name(dir))) {
			if (name[0] == '.')
				continue;
			if (name[strlen(name)-1] == '~')
				continue;
			g_ptr_array_add(ents, g_strdup(name));
		}
		g_dir_close(dir);
	}
	g_ptr_array_sort(ents, str_sort);
	g_ptr_array_add(ents, NULL);
	return (gchar**)g_ptr_array_free(ents, FALSE);
}

page_t *get_page(const char *path)
{
	char     *dir  = g_path_get_dirname(path);
	char     *base = g_path_get_basename(path);
	char     *ini  = g_build_filename(BASE, dir, "menu.ini", NULL);
	GKeyFile *conf = g_key_file_new();

	g_key_file_load_from_file(conf, ini, 0, NULL);
	//printf("load: %s -> %s\n", path, ini);

	page_t *page = g_new0(page_t, 1);

	page->title = g_key_file_get_string(conf, base, "title", NULL);
	page->keys  = g_key_file_get_string(conf, base, "keys",  NULL);
	page->desc  = g_key_file_get_string(conf, base, "desc",  NULL);

	g_key_file_free(conf);
	g_free(ini);
	g_free(base);
	g_free(dir);

	return page;
}

menu_t *get_menu_entry(const char *prefix, const char *entry)
{
	static GRegex *regex = NULL;
	if (regex == NULL)
		regex = g_regex_new("^([a-zA-Z0-9_-]+)\\.md$", 0, 0, NULL);

	GMatchInfo *matches = NULL;
	menu_t *menu = NULL;
	char *path = g_build_filename(BASE, prefix, entry, NULL);

	/* Test for directory */
	if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
		menu = g_new0(menu_t, 1);
		menu->path = g_build_filename(prefix, entry, "/", NULL);
		menu->name = g_strdup(entry);
		menu->base = g_strdup(entry);
		//printf("\tchild=[%s]\n", path);
	}

	/* Test for markdown */
	else if (g_file_test(path, G_FILE_TEST_IS_REGULAR) &&
	         g_regex_match(regex, entry, 0, &matches)) {
		char *name = g_match_info_fetch(matches, 1);
		menu = g_new0(menu_t, 1);
		menu->path = g_build_filename(prefix, name, NULL);
		menu->name = g_strdup(name);
		menu->base = g_strdup(name);
		for (char *c = menu->name; *c; c++)
			if (*c == '_')
				*c = ' ';
		//menu->name[0] = toupper(menu->name[0]);
		g_match_info_free(matches);
		//printf("\tname=[%s]\n", name);
	}

	g_free(path);
	return menu;
}

menu_t *get_menu_rec(char *prefix, char **parts)
{
	menu_t  head = {};
	menu_t *cur  = &head;
	menu_t *next = NULL;
	char **entries = NULL;

	char *path = g_build_filename(BASE, prefix, NULL);
	if (!g_file_test(path, G_FILE_TEST_IS_DIR))
		goto error;

	/* Load menu from key file */
	char     *ini  = g_build_filename(path, "menu.ini", NULL);
	GKeyFile *conf = g_key_file_new();
	g_key_file_load_from_file(conf, ini, 0, NULL);
	entries = g_key_file_get_groups(conf, NULL);
	for (int i = 0; entries[i]; i++) {
		next = get_menu_entry(prefix, entries[i]);
		if (!next) {
			next = g_new0(menu_t, 1);
			next->base = g_strdup(entries[i]);
		}
		char *pathval = g_key_file_get_string(conf, entries[i], "path", NULL);
		char *nameval = g_key_file_get_string(conf, entries[i], "name", NULL);
		int   hideval = g_key_file_get_boolean(conf, entries[i], "hide", NULL);
		if (pathval) next->path = pathval;
		if (nameval) next->name = nameval;
		if (!next->path) next->path = g_strdup(entries[i]);
		if (!next->name) next->name = g_strdup(entries[i]);
		if (hideval)
			next->show = SHOW_HIDDEN;
		if (!parts[0] && !strcmp(next->base, "index"))
			next->show = SHOW_ACTIVE;
		cur = cur->next = next;
	}
	g_strfreev(entries);
	g_free(ini);

	/* Load menu from directory entries */
	entries = lsdir(path);
	for (int i = 0; entries[i]; i++) {
		//printf("test: path=%s entry=%s\n", path, entries[i]);
		if (g_key_file_has_group(conf, entries[i]))
			continue;
		if ((next = get_menu_entry(prefix, entries[i])))
			cur = cur->next = next;
		if (!strcmp(cur->base, "index"))
			cur->show = SHOW_HIDDEN;
	}
	g_strfreev(entries);
	g_key_file_free(conf);

	/* Add child nodes */
	for (cur = head.next; cur; cur = cur->next) {
		if (parts && parts[0] && cur->base && !strcmp(cur->base, parts[0])) {
			//printf("test: [%s, %s == %s]\n",
			//	cur->path, cur->base, parts[0]);
			cur->kids = get_menu_rec(cur->path, parts+1);
			cur->show = SHOW_ACTIVE;
		}
	}

error:
	g_free(path);
	return head.next;
}

menu_t *get_menu(char *path)
{
#if 0
	static menu_t dev      = { "dev",      "Develop",  NULL,      NULL   };
	static menu_t news     = { "news",     "News",     &dev,      NULL   };
	static menu_t about    = { "about",    "About",    &news,     NULL   };

	static menu_t lackey   = { "lackey",   "lackey",   NULL,      NULL   };
	static menu_t wmpus    = { "wmpus",    "wmpus",    &lackey,   NULL   };
	static menu_t grits    = { "grits",    "Grits",    &wmpus,    &about };
	static menu_t aweather = { "aweather", "AWeather", &grits,    NULL   };
	static menu_t home     = { "",         "Home",     &aweather, NULL   };

	return &home;
#else
	char  **parts = g_strsplit(path, "/", -1);
	menu_t *menu = get_menu_rec("", parts);
	g_strfreev(parts);
	return menu;
#endif
}

int get_slashes(char *path)
{
	int slashes = 0;
	for (int i = 0; path[i]; i++) {
		if (path[i] == '/')
			slashes +=  1;
		if (path[i] != '/' && !path[i+1])
			slashes *= -1;
	}
	return slashes;
}

void print_menu(menu_t *menu, int first, int last)
{
	for (menu_t *cur = menu; cur; cur = cur->next) {
		if (first <= 0 && cur->show != SHOW_HIDDEN)
			print_link(cur->path, cur->name,
				cur->show == SHOW_ACTIVE,
				get_slashes(cur->path));
		if (cur->kids && last != 0) {
			if (first == 0)
				print_menu_start();
			print_menu(cur->kids, first-1, last-1);
			if (first == 0)
				print_menu_end();
		}
	}
}

void debug_menu(page_t *page, menu_t *menu, int depth)
{
	if (page) {
		printf("title: '%s'\n", page->title);
		printf("keys:  '%s'\n", page->keys);
		printf("desc:  '%s'\n", page->desc);
	}
	for (menu_t *cur = menu; cur; cur = cur->next) {
		for (int i = 0; i < depth; i++)
			printf("  ");
		printf("menu: %d %s %s '%s'\n",
			cur->show, cur->base, cur->path, cur->name);
		if (cur->kids)
			debug_menu(NULL, cur->kids, depth+1);
	}
}

void free_menu(menu_t *menu)
{
}

/* Page display methods */
static void do_lsdir(page_t *page, const char *dir)
{
	page->html = "Lsdir";
}

static void do_regular(page_t *page, const char *file)
{
	const char *query = getenv("QUERY_STRING") ?: "";
	if (g_file_get_contents(file, &page->text, NULL, NULL)) {
		if (!strcmp(query, "src"))
			page->html = "source";
		else if (!strcmp(query, "hist"))
			page->html = "history";
		else
			page->html = markdown_to_string(page->text, 0, 0);
	} else {
		page->error = g_strdup("403 Forbidden");
		page->html  = g_strdup("Page not accessable\n");
	}
}

static void do_notfound(page_t *page, const char *path)
{
	page->error = g_strdup("404 Not Found");
	page->html  = g_strdup("Page not found\n");
}

static char *clean(const char *src)
{
	int   len = strlen(src);
	char *dst = malloc(len+1);
	int si=0, ei=len-1, di=0;
	while (src[si] == '/')
		si++;
	while (src[ei] == '/')
		ei--;
	while (si <= ei) {
		dst[di++] = src[si++];
		while (src[si] == '/' && src[si+1] == '/')
			si++;
	}
	dst[di] = '\0';
	return dst;
}

/* Main */
int main(int argc, char **argv)
{
	char *path = clean(getenv("PATH_INFO") ?: "/");

	page_t *page = get_page(path);
	menu_t *menu = get_menu(path);
	//debug_menu(page, menu, 0);
	//return 0;

	char *dir   = g_strdup_printf("pages/%s",          path);
	char *index = g_strdup_printf("pages/%s/index.md", path);
	char *file  = g_strdup_printf("pages/%s.md",       path);

	if (g_file_test(index, G_FILE_TEST_IS_REGULAR))
		do_regular(page, index);
	else if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
		do_regular(page, file);
	else if (g_file_test(dir, G_FILE_TEST_IS_DIR))
		do_lsdir(page, dir);
	else
		do_notfound(page, path);

	g_free(dir);
	g_free(index);
	g_free(file);

	print_header(page);
	print_page(page, menu);

	return 0;
}
