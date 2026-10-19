# Third-party notices

The MIT license applies to Mirza Husadzic's project code and the 2026 port. It
does not replace the notices or terms attached to third-party contributions,
system dependencies or locally downloaded models.

## SGI trackball

src/trackball.h retains the original Silicon Graphics notice. The implementation
credits Gavin Bell, with ideas from Thant Tessman and the August 1988 issue of
Computer Graphics. Its vector code credits David M. Ciemiewicz, Mark Grossman,
Henry Moreton and Paul Haeberli. The permission notice is reproduced below as
required by the source:

```text
/*
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
```

## Bitfield helper

The `bitfield` / `bitfield_bit` helper in `src/gtkrtgutils.h` is by **tom vrx**,
included with permission. Its original credit is retained.

## Utah teapot

The README screenshot shows Martin Newell's Utah teapot, using the OBJ from
[common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models/blob/master/data/teapot.obj).
The model itself is downloaded separately and is not bundled with this project.
The project's MIT license does not extend to that external model.

## System libraries

GTK, GLib, Pango, Cairo, libepoxy and OpenGL are build/runtime dependencies,
not vendored source. Their own license terms apply when distributing binaries
or bundling their libraries.
