#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

enum {
	SEC = 1000
};

typedef struct Volume Volume;
struct Volume
{
	char dev[32];
	int fd;
	int lvl[2];
	int ismixfs;
};

Volume vol;
Image *knob;
Image *back;
Image *rim;

Point
volumept(Point c, int volume, int r)
{
	double rad;

	rad = (double)(volume*3.0 + 30) * PI/180.0;
	c.x -= sin(rad) * r;
	c.y += cos(rad) * r;
	return c;
}

void
redraw(void)
{
	Point c;
	char v[5]; /* nnn% */

	c = divpt(addpt(screen->r.min, screen->r.max), 2);
	snprint(v, sizeof(v), "%d%%", vol.lvl[0]);

	draw(screen, screen->r, back, nil, ZP);
	string(screen, addpt(c, Pt(0-stringwidth(font, v)/2, -32-font->height)), display->black, ZP, font, v);
	/* knob stops */
	line(screen, volumept(c, 0, 35), volumept(c, 0, 30), 0, 0, 1, display->black, ZP);
	line(screen, volumept(c, 100, 35), volumept(c, 100, 30), 0, 0, 1, display->black, ZP);
	/* knob */
	ellipse(screen, c, 30, 30, 1, rim, ZP);
	fillellipse(screen, volumept(c, vol.lvl[0], 20), 3, 3, knob, ZP);
	/* dev name */
	string(screen, addpt(c, Pt(0-stringwidth(font, vol.dev)/2, 32)), display->black, ZP, font, vol.dev);
}

void
update(void)
{
	char buf[256], *s, *e, *f[4];
	int n, nf;

	if((n = pread(vol.fd, buf, sizeof(buf)-1, 0)) <= 0)
		sysfatal("pread: %r");
	buf[n] = 0;

	s = buf;
	while((e = strchr(s, '\n')) != nil){
		*e++ = 0;

		nf = tokenize(s, f, nelem(f));
		if(nf < 2)
			break;

		if(strcmp(f[0], "dev") == 0){
			snprint(vol.dev, sizeof(vol.dev), "%s", f[1]);
			vol.ismixfs = 1;
		}else if(strcmp(f[0], "mix") == 0 || strcmp(f[0], "master") == 0){
			vol.lvl[0] = strtol(f[1], nil, 0);
			vol.lvl[1] = strtol(f[2], nil, 0);
		}
		s = e;
	}
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		fprint(2, "can't reattach to window");
	redraw();
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Mouse m;
	Event e;
	Point p;
	double rad;
	int Etimer, ev, d;

	ARGBEGIN{
	default: usage();
	}ARGEND;
	if(argc != 0)
		usage();

	vol.fd = open("/dev/volume", ORDWR);
	if(vol.fd < 0)
		sysfatal("open: %r");

	update();
	if(initdraw(0, 0, "volume") < 0)
		sysfatal("initdraw: %r");

	back = allocimagemix(display, 0x88FF88FF, DWhite);
	knob = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0x008800FF);
	rim = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0x004400FF);

	einit(Emouse);
	Etimer = etimer(0, 2*SEC);
	redraw();

	for(;;){
		ev = event(&e);
		if(ev == Emouse){
			m = e.mouse;
			if(!(m.buttons & (1|8|16)))
				continue;
			if(m.buttons & 1){
				p = subpt(m.xy, divpt(addpt(screen->r.min, screen->r.max), 2));
				rad = atan2(-p.x, p.y);
				d = rad * 180.0/PI;
				if(d < 0)
					d += 360;
				d *= 160.0/360.0;
				if(d < 30)
					d = 0;
				else if(d > 130)
					d = 100;
				else
					d -= 30;
				vol.lvl[0] = d;
			}
			if(m.buttons & 8 && vol.lvl[0] < 100)
				vol.lvl[0]++;
			if(m.buttons & 16 && vol.lvl[0] > 0)
				vol.lvl[0]--;
			fprint(vol.fd, "%s%d", vol.ismixfs? "mix " : "", vol.lvl[0]);
			redraw();
		}else if(ev == Etimer){
			update();
			redraw();
		}
	}
}
