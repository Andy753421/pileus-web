/* Links */
#define PILEUS   "http://pileus.org/"

#define DEV_LIST "http://pileus.org/mailman/listinfo/dev"
#define DEV_ARCH "http://pileus.org/pipermail/dev/"

/* Constants */
#define BASE "pages"

/* Forward declarations */
typedef struct page_t page_t;
typedef struct menu_t menu_t;

/* Menu display display types */
typedef enum {
	SHOW_NORMAL,
	SHOW_ACTIVE,
	SHOW_HIDDEN,
} show_t;

/* Page information */
struct page_t {
	char     *title; // title tag
	char     *keys;  // meta keywords tag, or NULL
	char     *desc;  // meta description tag, or NULL
	char     *error; // http status
	char     *text;  // unfiltered text
	char     *html;  // generated html
};

/* Navigation menu entry */
struct menu_t {
	char     *path;  // path to the page
	char     *name;  // name of the page
	char     *base;  // base file name of the page
	menu_t   *next;  // next menu item
	menu_t   *kids;  // child menu items for directories
	show_t    show;  // is this part of the current path?
};

/* Helper functions */
void href(const char *url);

void print_link(char *path, char *name, int cur, int dir);
void print_menu(menu_t *menu, int first, int last);
void print_menu_start(void);
void print_menu_end(void);

/* Global functions */
void print_header(page_t *page);
void print_page(page_t *page, menu_t *menu);
