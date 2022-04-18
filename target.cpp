// Fishlet Shooting Targets in PDF using the Cairo drawing library
// (c) 2022 Curt McDowell

#include <iostream>
#include <sstream>
#include <cassert>

#include <math.h>
#include <string.h>
#include <getopt.h>

#include <cairo.h>
#include <cairo-pdf.h>

using namespace std;

const char *DEFAULT_GEOM = "8.5x11";
const double DEFAULT_MARGIN = 0.25;
const char *DEFAULT_FNAME = "target.pdf";
const int DEFAULT_RINGS = 8;
const int DEFAULT_ORINGS = 2;
const int DEFAULT_IRINGS = 3;
const double DEFAULT_LINEW = 0.05;

const char *FISH_IMAGE = "koi.png";

double
inch_pt(double i)
{
    return i * 72.0;
}

double
pt_inch(double p)
{
    return p / 72.0;
}

constexpr int
gcd(int a, int b)
{
    return (a == 0) ? b : (a < b) ? gcd(a, b % a) : gcd(b, a);
}

void
usage()
{
    cerr << "Usage: target [options]\n";
    cerr << "   -s WxH       Set size in inches (" << DEFAULT_GEOM << ")\n";
    cerr << "   -m MARGIN    Set page margin (" << DEFAULT_MARGIN << ")\n";
    cerr << "   -o FNAME     Set output filename (" << DEFAULT_FNAME << ")\n";
    cerr << "   -r RINGS     Set number of rings (" << DEFAULT_RINGS << ")\n";
    cerr << "   -I IRINGS    Set number of inner rings (" << DEFAULT_IRINGS << ")\n";
    cerr << "   -O ORINGS    Set number of outer rings (" << DEFAULT_ORINGS << ")\n";
    cerr << "   -l LINEW     Set line width (" << DEFAULT_LINEW << ")\n";
    cerr << "   -b           Use yellowish background color\n";
    exit(2);
}

const unsigned int ALIGN_H_CENTER = 1;
const unsigned int ALIGN_H_LEFT = 2;
const unsigned int ALIGN_H_RIGHT = 4;

const unsigned int ALIGN_V_CENTER = 8;
const unsigned int ALIGN_V_TOP = 16;
const unsigned int ALIGN_V_BOTTOM = 32;

void
aligned_text(cairo_t *cr, double x, double y, unsigned int align, const char *text)
{
    cairo_text_extents_t te;
    cairo_text_extents(cr, text, &te);

    double dx = ((align & ALIGN_H_CENTER) ? -te.width / 2 :
		 (align & ALIGN_H_RIGHT) ? -te.width : 0);
    double dy = ((align & ALIGN_V_CENTER) ? te.height / 2 :
		 (align & ALIGN_V_TOP) ? te.height : 0);

    cairo_save(cr);
    cairo_move_to(cr, x + dx, y + dy);
    cairo_show_text(cr, text);
    cairo_restore(cr);
}

struct fish {
    fish() {
	im = cairo_image_surface_create_from_png(FISH_IMAGE);
	cairo_status_t status = cairo_surface_status(im);
	if (status != 0) {
	    cerr << "Could not load image " << FISH_IMAGE << ": " <<
		cairo_status_to_string(status) << "\n";
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
target_eye(cairo_t *cr, double x, double y, double r)
{
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_arc(cr, x, y, r, 0.0, 2 * M_PI);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_stroke(cr);
}

void
check_status(cairo_t *cr)
{
    cairo_status_t status = cairo_status(cr);
    if (status != 0) {
	cerr << "Operation failed: " << cairo_status_to_string(status) << "\n";
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

    double width = inch_pt(atof(opt_geom));
    double height = inch_pt(atof(s + 1));
    double margin = inch_pt(opt_margin);
    double cx = width / 2;
    double cy = height / 2;
    double linew = inch_pt(opt_linew);

    //    ((   ((   ((   o   ))   ))   ))
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
	if (ring <= opt_irings || ring > opt_rings - opt_orings)
	    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	else
	    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

	ostringstream num_buf;
	num_buf << ring;
	const string num_str = num_buf.str();
	const char *num_s = num_str.c_str();

	aligned_text(cr, cx + ring * rs, cy, ALIGN_H_CENTER | ALIGN_V_CENTER, num_s);
	aligned_text(cr, cx - ring * rs, cy, ALIGN_H_CENTER | ALIGN_V_CENTER, num_s);
	aligned_text(cr, cx, cy + ring * rs, ALIGN_H_CENTER | ALIGN_V_CENTER, num_s);
	aligned_text(cr, cx, cy - ring * rs, ALIGN_H_CENTER | ALIGN_V_CENTER, num_s);
    }

    cairo_new_path(cr);

    // Four extra target eyes
    int eye_ring = opt_rings - 1;
    double tr = ring_radius(radius, opt_rings, eye_ring);
    double td = tr * sqrt(2.0) / 2;

    target_eye(cr, cx - td, cy - td, rs / 2);
    target_eye(cr, cx + td, cy - td, rs / 2);
    target_eye(cr, cx + td, cy + td, rs / 2);
    target_eye(cr, cx - td, cy + td, rs / 2);

    // Koi decorations
    fish *f = new fish;

    double image_inches = 2.0;
    double image_width = inch_pt(image_inches);
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
    int num = (int)(pt_inch(rs) * 32 + 0.5);
    int g = gcd(num, den);

    ostringstream rs_buf;
    rs_buf << "Ring spacing " << (num / g) << "/" << (den / g) << "\"";
    const string rs_str = rs_buf.str();
    const char *rs_s = rs_str.c_str();

    cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_size);

    aligned_text(cr, margin + image_width / 2, margin + image_height + font_size,
		 ALIGN_H_CENTER | ALIGN_V_TOP, "www.fishlet.com");
    aligned_text(cr, width - margin - image_width / 2, margin + image_height + font_size,
		 ALIGN_H_CENTER | ALIGN_V_TOP, "www.fishlet.com");
    aligned_text(cr, margin + image_width / 2, height - margin - image_height - font_size,
		 ALIGN_H_CENTER | ALIGN_V_BOTTOM, rs_s);
    aligned_text(cr, width - margin - image_width / 2, height - margin - image_height - font_size,
		 ALIGN_H_CENTER | ALIGN_V_BOTTOM, "Copyright © 2022");

    // Must clean up after show page or file won't be complete
    cairo_show_page(cr);
    check_status(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return 0;
}
