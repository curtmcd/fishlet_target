// Fishlet Shooting Targets in PDF using the Cairo drawing library
// (c) 2022 Curt McDowell

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <getopt.h>

#include <cairo.h>
#include <cairo-pdf.h>

#define DEFAULT_GEOM		"8.5x11"
#define DEFAULT_MARGIN		0.25
#define DEFAULT_FNAME		"target.pdf"
#define DEFAULT_RINGS		8
#define DEFAULT_ORINGS		2
#define DEFAULT_IRINGS		3
#define DEFAULT_LINEW		0.05

#define INCH_PT(i)		((i) * 72.0)
#define PT_INCH(i)		((i) / 72.0)

#define FISH_IMAGE		"koi.png"

int
gcd(int a, int b)
{
    return (a == 0) ? b : (a < b) ? gcd(a, b % a) : gcd(b, a);
}

void
perr(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
}

void
usage()
{
    perr("Usage: target [options]\n");
    perr("   -s WxH       Set size in inches, default %s\n", DEFAULT_GEOM);
    perr("   -m MARGIN    Set page margin, default %g\n", DEFAULT_MARGIN);
    perr("   -o FNAME     Set output filename, default %s\n", DEFAULT_FNAME);
    perr("   -r RINGS     Set number of rings, default %d\n", DEFAULT_RINGS);
    perr("   -I IRINGS    Set number of inner rings, default %d\n", DEFAULT_IRINGS);
    perr("   -O ORINGS    Set number of outer rings, default %d\n", DEFAULT_ORINGS);
    perr("   -l LINEW     Set line width, default %d\n", DEFAULT_LINEW);
    perr("   -b           Use yellowish background color\n");
    exit(2);
}

#define ALIGN_H_CENTER		1
#define ALIGN_H_LEFT		2
#define ALIGN_H_RIGHT		4

#define ALIGN_V_CENTER		8
#define ALIGN_V_TOP		16
#define ALIGN_V_BOTTOM		32

void
aligned_text(cairo_t *cr, double x, double y, unsigned int align, const char *text)
{
    cairo_text_extents_t te;
    cairo_text_extents(cr, text, &te);

    double dx = 0;
    if (align & ALIGN_H_CENTER)
	dx = -te.width / 2;
    else if (align & ALIGN_H_RIGHT)
	dx = -te.width;

    double dy = 0;
    if (align & ALIGN_V_CENTER)
	dy = te.height / 2;
    else if (align & ALIGN_V_TOP)
	dy = te.height;

    cairo_save(cr);
    cairo_move_to(cr, x + dx, y + dy);
    cairo_show_text(cr, text);
    cairo_restore(cr);
}

void
target_eye(cairo_t *cr, double x, double y, double r)
{
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_arc(cr, x, y, r, 0.0, 2 * M_PI);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_stroke(cr);
}

struct fish {
    fish() {
	im = cairo_image_surface_create_from_png(FISH_IMAGE);
	cairo_status_t status = cairo_surface_status(im);
	if (status != 0) {
	    perr("Could not load image %s: %s\n",
		 FISH_IMAGE, cairo_status_to_string(status));
	    exit(1);
	}
	cairo_t *im_cr;
	double ulx, uly, lrx, lry;
	im_cr = cairo_create(im);
	cairo_clip_extents(im_cr, &ulx, &uly, &lrx, &lry);
	cairo_destroy(im_cr);
	im_w = lrx - ulx;
	im_h = lry - uly;
	width = 0.0;
    }

    ~fish() {
	cairo_surface_destroy(im);
    }

    void width_set(double w) {
	width = w;
    }

    double height_get() {
	return width * im_h / im_w;
    }

    void put(cairo_t *cr, double x, double y) {
	assert(width != 0.0);
	double s = width / im_w;
	cairo_save(cr);
	cairo_translate(cr, x, y);
	cairo_scale(cr, s, s);
	cairo_set_source_surface(cr, im, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);
    }

    cairo_surface_t *im;
    double im_w, im_h;
    double width;
};

void
check_status(cairo_t *cr)
{
    cairo_status_t status = cairo_status(cr);
    if (status != 0) {
	perr("Operation failed: %s\n", cairo_status_to_string(status));
	exit(1);
    }
}

double
ring_spacing(double radius, int rings)
{
    return radius / (rings + 0.5);
}

double
ring_radius(double radius, int rings, int ring) {
    double rs = ring_spacing(radius, rings);
    return rs / 2 + ring * rs;
}

int
main(int argc, char *argv[])
{
    const char *opt_geom = DEFAULT_GEOM;
    double opt_margin = DEFAULT_MARGIN;
    const char *opt_fname = DEFAULT_FNAME;
    int opt_rings = DEFAULT_RINGS;
    int opt_irings = DEFAULT_IRINGS;
    int opt_orings = DEFAULT_ORINGS;
    double opt_linew = DEFAULT_LINEW;
    bool opt_bg;

    int opt;
    while ((opt = getopt(argc, argv, "s:m:o:r:I:O:l:b")) >= 0)
	switch (opt) {
	case 's':
	    opt_geom = optarg;
	    break;
	case 'm':
	    opt_margin = atof(optarg);
	    break;
	case 'o':
	    opt_fname = optarg;
	    break;
	case 'r':
	    opt_rings = atoi(optarg);
	    break;
	case 'I':
	    opt_irings = atoi(optarg);
	    break;
	case 'O':
	    opt_orings = atoi(optarg);
	    break;
	case 'l':
	    opt_linew = atof(optarg);
	    break;
	case 'b':
	    opt_bg = true;
	    break;
	default:
	    usage();
	}

    const char *s = strchr(opt_geom, 'x');
    if (s == NULL)
	usage();

    double width = INCH_PT(atof(opt_geom));
    double height = INCH_PT(atof(s + 1));
    double margin = INCH_PT(opt_margin);
    double cx = width / 2;
    double cy = height / 2;
    double linew = INCH_PT(opt_linew);

    //    ((    ((    (( c ))    ))    ))
    //    |<-- radius -->|
    int radius;			// Radius of outside of outer ring
    if (width < height)
	radius = width / 2 - margin - linew / 2;
    else
	radius = height / 2 - margin - linew / 2;

    cairo_surface_t *surface = cairo_pdf_surface_create(opt_fname, width, height);
    cairo_t *cr = cairo_create(surface);
    check_status(cr);

    //cairo_pdf_surface_set_page_label(surface, utf8_string);

    if (opt_bg) {
	cairo_rectangle(cr,
			margin, margin,
			width - 2 * margin, height - 2 * margin);
	cairo_set_source_rgba(cr, 0.95, 0.95, 0.8, 1.0);
	cairo_fill(cr);
    }

    cairo_set_line_width(cr, linew);

    // Large blue disk
    cairo_set_source_rgba(cr, 0.3, 0.5, 1.0, 1.0);
    cairo_arc(cr, cx, cy, ring_radius(radius, opt_rings, opt_rings), 0, 2 * M_PI);
    cairo_fill(cr);

    // Overlay medium white disk
    cairo_set_source_rgba(cr, 10.0, 1.0, 1.0, 1.0);
    cairo_arc(cr, cx, cy, ring_radius(radius, opt_rings, opt_rings - opt_orings), 0, 2 * M_PI);
    cairo_fill(cr);

    // Overlay red small disk
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    cairo_arc(cr, cx, cy, ring_radius(radius, opt_rings, opt_irings), 0, 2 * M_PI);
    cairo_fill(cr);

    // Overlay white bullseye disk
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_arc(cr, cx, cy, ring_radius(radius, opt_rings, 0), 0, 2 * M_PI);
    cairo_fill(cr);

    // Draw concentric rings in black or white as necessary for contrast
    for (int ring = 0; ring <= opt_rings; ring++) {
	double r = ring_radius(radius, opt_rings, ring);
	cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
	if ((ring > 0 && ring < opt_irings) ||
	    (ring > opt_rings - opt_orings && ring < opt_rings))
	    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	else
	    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_stroke(cr);
    }

    // Ring numbers
    double rs = ring_spacing(radius, opt_rings);

    cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, rs / 2);

    for (int ring = 1; ring <= opt_rings; ring++) {
	char buf[80];
	snprintf(buf, sizeof (buf), "%d", ring);
	if (ring <= opt_irings || ring > opt_rings - opt_orings)
	    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	else
	    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	aligned_text(cr, cx + ring * rs, cy, ALIGN_H_CENTER | ALIGN_V_CENTER, buf);
	aligned_text(cr, cx - ring * rs, cy, ALIGN_H_CENTER | ALIGN_V_CENTER, buf);
	aligned_text(cr, cx, cy + ring * rs, ALIGN_H_CENTER | ALIGN_V_CENTER, buf);
	aligned_text(cr, cx, cy - ring * rs, ALIGN_H_CENTER | ALIGN_V_CENTER, buf);
    }

    cairo_new_path(cr);

    // Four extra target eyes
    //double tr = ring_radius(radius, opt_rings, opt_rings - 0);
    double tr = ring_radius(radius, opt_rings, opt_rings - 1);
    double td = tr * sqrt(2.0) / 2;

    target_eye(cr, cx - td, cy - td, rs / 2);
    target_eye(cr, cx + td, cy - td, rs / 2);
    target_eye(cr, cx + td, cy + td, rs / 2);
    target_eye(cr, cx - td, cy + td, rs / 2);

    // Koi decorations
    fish *f = new fish;

    double image_inches = 2.0;
    double image_width = INCH_PT(image_inches);		// in points
    f->width_set(image_width);
    double image_height = f->height_get();

    f->put(cr, margin, margin);
    f->put(cr, width - margin - image_width, margin);
    f->put(cr, margin, height - margin - image_height);
    f->put(cr, width - margin - image_width, height - margin - image_height);

    delete f;

    // Additional labels
    int font_size = 12;
    int den = 32;
    int num = (int)(PT_INCH(rs) * 32 + 0.5);
    int g = gcd(num, den);
    char rs_buf[80];

    snprintf(rs_buf, sizeof (rs_buf), "Ring spacing %d/%d\"", num / g, den / g);

    cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_size);

    aligned_text(cr, margin + image_width / 2, margin + image_height + font_size,
		 ALIGN_H_CENTER | ALIGN_V_TOP, "www.fishlet.com");
    aligned_text(cr, width - margin - image_width / 2, margin + image_height + font_size,
		 ALIGN_H_CENTER | ALIGN_V_TOP, "www.fishlet.com");
    aligned_text(cr, margin + image_width / 2, height - margin - image_height - font_size,
		 ALIGN_H_CENTER | ALIGN_V_BOTTOM, rs_buf);
    aligned_text(cr, width - margin - image_width / 2, height - margin - image_height - font_size,
		 ALIGN_H_CENTER | ALIGN_V_BOTTOM, "Copyright © 2022");

    // Must clean up after show page or file won't be complete
    cairo_show_page(cr);
    check_status(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return 0;
}
