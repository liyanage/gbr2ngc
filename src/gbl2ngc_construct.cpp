/*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* This program was written while working at Bright Works Computer Consulting
* and allowed to be GPL'd under express permission of the current president
* John Guttridge
* Dated May 20th 2013
*/

#include "gbl2ngc.hpp"

#define DEBUG_CONSTRUCT

class Gerber_point_2 {
  public:
    int ix, iy;
    double x, y;
    int jump_pos;
};

class irange_2 {
  public:
    int start, end;
};

typedef struct dpoint_2_type {
  int ix, iy;
  double x, y;
} dpoint_2_t;

struct Gerber_point_2_cmp {

  bool operator()(const Gerber_point_2 &lhs, const Gerber_point_2 &rhs) const {
    if (lhs.ix < rhs.ix) return true;
    if (lhs.ix > rhs.ix) return false;
    if (lhs.iy < rhs.iy) return true;
    return false;
  }

};

typedef std::map< Gerber_point_2, int, Gerber_point_2_cmp  > PointPosMap;
typedef std::pair< Gerber_point_2, int > PointPosMapPair;

typedef std::map< Gerber_point_2, int , Gerber_point_2_cmp  > HolePosMap;


// add a hole to the vector hole_vec from the Gerber_point_2
// vector p.  The points are decorated with a 'jump_pos' which
// holds the position of the first time the path comes back
// to the same point.  The path can come back to the same
// point more than twice, so we need to skip over the duplicate
// points.
//
// Here is some ASCII art to illustrate:
//
//            >...     ...>
//                \   /
//                 a a
//                 | |
//                 | |
//            <----b b-----<
//
// where a is a distinct point from b and the 'a' points overlap
// and the 'b' points overlap.  The hole really starts (and ends)
// at point 'a'.  The outer region connects to the inner hole
// through point 'b'.
// start and end hold the positions the starting 'a' and ending 'a'
// point in the p vector.
//
void add_hole( Paths &hole_vec,
               std::vector< Gerber_point_2 > &p,
               int start,
               int n ) {
  int i, ds, jp, end;
  Path hole_polygon;

  end = start+n;

  jp = p[start].jump_pos;

  if (jp<0) {
    fprintf(stderr, "ERROR: jump pos MUST be > 0, is %i, exiting (start %i, n %i)\n", jp, start, n );
    printf("ERROR: jump pos MUST be > 0, is %i, exiting (start %i, n %i)\n", jp, start, n );
    exit(2);
  }

  while ( p[jp].jump_pos >= 0 ) {
    ds = p[start].jump_pos - (start+1);
    add_hole( hole_vec, p, start+1, ds );
    start = jp;
    jp = p[start].jump_pos;
  }

  n = end-start;
  hole_polygon.push_back( dtoc(p[start].x, p[start].y) );

  for (i=start+1; i<(start+n); i++) {
    hole_polygon.push_back( dtoc(p[i].x, p[i].y) );
    if (p[i].jump_pos<0) continue;

    ds = p[i].jump_pos - (i+1);
    add_hole( hole_vec, p, i+1, ds);
    i = p[i].jump_pos;

  }

  if (hole_polygon.size() > 2)
    hole_vec.push_back( hole_polygon );

}



/*
// Copy linked list points in contour to vector p, removing contiguougs
// duplicates.
//
void populate_gerber_point_vector_from_contour( std::vector< Gerber_point_2 > &p,
                                                contour_ll_t *contour) {
  int first=1;
  Gerber_point_2 prev_pnt;
  Gerber_point_2 dpnt;

  // Get rid of points that are duplicated next to each other.
  // It does no good to have them and it just makes things more
  // complicated downstream to work around them.
  //
  while (contour) {
    //dpnt.ix = int(contour->x * 10000.0);
    //dpnt.iy = int(contour->y * 10000.0);
    dpnt.ix = int(contour->x * 1000000.0);
    dpnt.iy = int(contour->y * 1000000.0);
    dpnt.x = contour->x;
    dpnt.y = contour->y;

    if (first) {
      p.push_back( dpnt );
    }
    else if ( (prev_pnt.ix == dpnt.ix) &&
              (prev_pnt.iy == dpnt.iy) ) {
      // duplicate
    }
    else {
      p.push_back( dpnt );
    }

    prev_pnt = dpnt;
    first = 0;

    contour = contour->next;
  }

}
*/

// Copy linked list points in contour to vector p, removing contiguougs
// duplicates.
//
void populate_gerber_point_vector_from_contour( std::vector< Gerber_point_2 > &p,
                                                gerber_region_t *contour) {
  int first=1;
  Gerber_point_2 prev_pnt;
  Gerber_point_2 dpnt;

  // Get rid of points that are duplicated next to each other.
  // It does no good to have them and it just makes things more
  // complicated downstream to work around them.
  //
  while (contour) {
    //dpnt.ix = int(contour->x * 10000.0);
    //dpnt.iy = int(contour->y * 10000.0);
    dpnt.ix = int(contour->x * 1000000.0);
    dpnt.iy = int(contour->y * 1000000.0);
    dpnt.x = contour->x;
    dpnt.y = contour->y;

    if (first) {
      p.push_back( dpnt );
    }
    else if ( (prev_pnt.ix == dpnt.ix) &&
              (prev_pnt.iy == dpnt.iy) ) {
      // duplicate
    }
    else {
      p.push_back( dpnt );
    }

    prev_pnt = dpnt;
    first = 0;

    contour = contour->next;
  }

}


// Create the start and end positions of the holes in the vector p.
// Store in hole_map.
//
int gerber_point_2_decorate_with_jump_pos( std::vector< Gerber_point_2 > &p ) {
  unsigned int i;
  int k;
  HolePosMap hole_map;

  for (i=0; i<p.size(); i++) {
    p[i].jump_pos = -1;

    // if found in p_map, add it to our map
    //
    if ( hole_map.find( p[i] ) != hole_map.end() ) {
      k = hole_map[ p[i] ];
      p[ k ].jump_pos = i;

      hole_map[ p[i] ] = i;
    }

    // otherwise add it
    //
    else {
      hole_map[ p[i] ] = i;
    }

  }

  return 0;
}


//----------------------------------
//
//

/*
// Very similar to add_hole but special care needs to be taken for the outer
// contour.
// Construct the point vector and hole map.  Create the polygon with holes
// vector.
//
int construct_contour_region( PathSet &pwh_vec, contour_ll_t *contour ) {
  int i, ds;

  std::vector< Gerber_point_2 > p;

  Path path;
  Paths pwh, soln;

  Clipper clip;

  // Initially populate p vector
  //
  populate_gerber_point_vector_from_contour( p, contour );

  // Find the start and end regions for each of the
  // holes.
  //
  gerber_point_2_decorate_with_jump_pos( p );

  if (p.size()==0) { return -1; }

  int n = p.size();
  for (i=0; i<n; i++) {

    path.push_back( dtoc( p[i].x, p[i].y ) );
    if (p[i].jump_pos < 0) { continue; }

    // Special case when the boundary end ties back to the beginning
    // without any jumps to holes.
    //
    if (p[i].jump_pos == (n-1)) { continue; }

    ds = p[i].jump_pos - (i+1);
    add_hole( pwh, p, i+1, ds );
    i += ds;

  }
  pwh.push_back(path);

  // The contour doesn't necessarily start on the outer boundary,
  // so we need to do an even/odd union to get the proper
  // region.
  //
  clip.AddPaths( pwh, ptSubject, true );
  clip.Execute( ctUnion, soln, pftEvenOdd, pftEvenOdd );

  pwh_vec.push_back( soln );

  return 0;
}
*/


// Very similar to add_hole but special care needs to be taken for the outer
// contour.
// Construct the point vector and hole map.  Create the polygon with holes
// vector.
//
int construct_contour_region( PathSet &pwh_vec, gerber_region_t *contour ) {
  int i, ds;

  std::vector< Gerber_point_2 > p;

  Path path;
  Paths pwh, soln;

  Clipper clip;

  // Initially populate p vector
  //
  populate_gerber_point_vector_from_contour( p, contour );

  // Find the start and end regions for each of the
  // holes.
  //
  gerber_point_2_decorate_with_jump_pos( p );

  if (p.size()==0) { return -1; }

  int n = p.size();
  for (i=0; i<n; i++) {

    path.push_back( dtoc( p[i].x, p[i].y ) );
    if (p[i].jump_pos < 0) { continue; }

    // Special case when the boundary end ties back to the beginning
    // without any jumps to holes.
    //
    if (p[i].jump_pos == (n-1)) { continue; }

    ds = p[i].jump_pos - (i+1);
    add_hole( pwh, p, i+1, ds );
    i += ds;

  }
  pwh.push_back(path);

  // The contour doesn't necessarily start on the outer boundary,
  // so we need to do an even/odd union to get the proper
  // region.
  //
  clip.AddPaths( pwh, ptSubject, true );
  clip.Execute( ctUnion, soln, pftEvenOdd, pftEvenOdd );

  pwh_vec.push_back( soln );

  return 0;
}


//-----------------------------------------

bool isccw( Path &p ) {
  unsigned int i;
  cInt dx, dy, s = 0;

  for (i=1; i<p.size(); i++) {
    dx = p[i].X - p[0].X;
    dy = p[i].Y - p[0].Y;
    s += dx*dy;
  }

  if (s < 0) return true;
  return false;
}

/*
void print_polygon_set(gerber_state_t *gs) {
  unsigned int i, j, k;
  contour_list_ll_t *contour_list;
  contour_ll_t      *contour, *prev_contour;

  PathSet temp_pwh_vec;
  IntPoint prev_pnt, cur_pnt;

  for (contour_list = gs->contour_list_head;
       contour_list;
       contour_list = contour_list->next) {
    prev_contour = NULL;
    contour = contour_list->c;

    if (contour->region) {
      construct_contour_region(temp_pwh_vec, contour);
      continue;
    }

    while (contour) {
      if (!prev_contour) {
        prev_contour = contour;
        continue;
      }

      Path point_list;
      Path res_point;
      int name = contour->d_name;

      prev_pnt = dtoc( prev_contour->x, prev_contour->y );
      cur_pnt = dtoc( contour->x, contour->y );

      // An informative example:
      // Consider drawing a line with rounded corners from position prev_contour->[xy] to contour->[xy].
      // This is done by first drawing a (linearized) circle (in general, an aperture) at prev_contour,
      // then drawing another circle at contour.  After those two are formed, take the convex hull
      // of the two (this is the ch_graham_andrew call).  Now we have a "line" with width, which is
      // really a polygon that we've constructed.
      // After all these polygoin segments have been constructed, do a big join at the end to eliminate
      // overlap.
      //

      ConvexHull( point_list, res_point );

      if (res_point[ res_point.size() - 1] == res_point[0]) {
        res_point.pop_back();
      }

      if (Area(res_point) < 0.0) {
        std::reverse( res_point.begin(), res_point.end() );
      }

      if (res_point.size()>2) {
        printf("\n\n");
        for (i=0; i<res_point.size(); i++) {
          printf("%lli %lli\n",
              res_point[i].X,
              res_point[i].Y);
        }
        printf("%lli %lli\n",
            res_point[0].X,
            res_point[0].Y);
      } else {
        printf("## res_point size %i\n", (int)res_point.size());
      }

      contour = contour->next;
    }

  }

  for (i=0; i<temp_pwh_vec.size(); i++) {
    for (j=0; j<temp_pwh_vec[i].size(); j++) {
      printf("\n\n");
      printf("## polygon with hole vector %i, %i\n", i, j);
      for (k=0; k<temp_pwh_vec[i][j].size(); k++) {
        printf("%lli %lli\n",
            temp_pwh_vec[i][j][k].X,
            temp_pwh_vec[i][j][k].Y);
      }
      printf("%lli %lli\n",
          temp_pwh_vec[i][j][0].X,
          temp_pwh_vec[i][j][0].Y);
    }
  }

}
*/

/*
void print_polygon_set(gerber_state_t *gs) {
  unsigned int i, j, k;
  gerber_region_t *region, *region_prv;
  gerber_item_ll_t *item_nod, *item_pr;

  PathSet temp_pwh_vec;
  IntPoint prev_pnt, cur_pnt;

  for (item_nod = gs->item_head;
       item_nod;
       item_nod = item_nod->next) {

    if (item_nod->type == GERBER_REGION) {
      construct_contour_region(temp_pwh_vec, item_nod->region);
      continue;
    }

    while (contour) {
      if (!prev_contour) {
        prev_contour = contour;
        continue;
      }

      Path point_list;
      Path res_point;
      int name = contour->d_name;

      prev_pnt = dtoc( prev_contour->x, prev_contour->y );
      cur_pnt = dtoc( contour->x, contour->y );

      // An informative example:
      // Consider drawing a line with rounded corners from position prev_contour->[xy] to contour->[xy].
      // This is done by first drawing a (linearized) circle (in general, an aperture) at prev_contour,
      // then drawing another circle at contour.  After those two are formed, take the convex hull
      // of the two (this is the ch_graham_andrew call).  Now we have a "line" with width, which is
      // really a polygon that we've constructed.
      // After all these polygoin segments have been constructed, do a big join at the end to eliminate
      // overlap.
      //

      for (i=0; i<gAperture[ name ].m_outer_boundary.size(); i++) {
        point_list.push_back( IntPoint(  gAperture[ name ].m_outer_boundary[i].X + prev_pnt.X,
                                         gAperture[ name ].m_outer_boundary[i].Y + prev_pnt.Y  ) );
      }

      for (i=0; i<gAperture[ name ].m_outer_boundary.size(); i++) {
        point_list.push_back( IntPoint( gAperture[ name ].m_outer_boundary[i].X + cur_pnt.X ,
                                        gAperture[ name ].m_outer_boundary[i].Y + cur_pnt.Y ) );
      }

      ConvexHull( point_list, res_point );

      if (res_point[ res_point.size() - 1] == res_point[0]) {
        res_point.pop_back();
      }

      if (Area(res_point) < 0.0) {
        std::reverse( res_point.begin(), res_point.end() );
      }

      if (res_point.size()>2) {
        printf("\n\n");
        for (i=0; i<res_point.size(); i++) {
          printf("%lli %lli\n",
              res_point[i].X,
              res_point[i].Y);
        }
        printf("%lli %lli\n",
            res_point[0].X,
            res_point[0].Y);
      } else {
        printf("## res_point size %i\n", (int)res_point.size());
      }

      contour = contour->next;
    }

  }

  for (i=0; i<temp_pwh_vec.size(); i++) {
    for (j=0; j<temp_pwh_vec[i].size(); j++) {
      printf("\n\n");
      printf("## polygon with hole vector %i, %i\n", i, j);
      for (k=0; k<temp_pwh_vec[i][j].size(); k++) {
        printf("%lli %lli\n",
            temp_pwh_vec[i][j][k].X,
            temp_pwh_vec[i][j][k].Y);
      }
      printf("%lli %lli\n",
          temp_pwh_vec[i][j][0].X,
          temp_pwh_vec[i][j][0].Y);
    }
  }

}
*/

static void debug_gAperture() {
  int key;
  Aperture_realization *rlz;
  ApertureNameMap::iterator it;

  for (it = gAperture.begin(); it != gAperture.end(); ++it ) {
    key = it->first;
    rlz = &(it->second);

    printf("## debug_gAperture: key:%i\n", key);
  }

}

/*
// construct the final polygon set (before offsetting)
//   of the joined polygons from the parsed gerber file.
//
// The aperture macros add a lot of complexity, so the basic
// idea to construct the geometry is:
//
// * if it's a 'region', add to a running `clip` of subject
//   for a union
// * for individual elements, construct a temporry `cur_clip`
//   that does an individual subject union for exposure of 1
//   and a differenc clip of exposure 0
//
void join_polygon_set(Paths &result, gerber_state_t *gs) {

  unsigned int i, ii, jj;
  contour_list_ll_t *contour_list;
  contour_ll_t      *contour, *prev_contour;

  PathSet temp_pwh_vec;
  IntPoint prev_pnt, cur_pnt;

  Paths clip_paths, clip_result;
  Path clip_path;

  Clipper clip;
  int n=0;

  int name=0;
  int _path_polarity=1;

  Path point_list, res_point, tmp_path;
  Paths aperture_geom, it_paths;
  Clipper aperture_clip;

  //DEBUG
  printf("## join_polygon_set\n");
  debug_gAperture();

  printf("## gs %i [%i] (%p), gs->contour_list_head %p\n",
      gs->id, gs->name, gs,

      gs->contour_list_head);

  for (contour_list = gs->contour_list_head;
       contour_list;
       contour_list = contour_list->next) {
    prev_contour = NULL;
    contour = contour_list->c;

    //DEBUG
    printf("## cp...\n");

    if (contour->region) {

      //DEBUG
      printf("## cp.region ...\n");

      temp_pwh_vec.clear();
      construct_contour_region(temp_pwh_vec, contour);

      for (i=0; i<temp_pwh_vec.size(); i++) {
        clip.AddPaths( temp_pwh_vec[i], ptSubject, true );
      }

      continue;
    }

    n=0;
    while (contour) {

      n++;
      if (!prev_contour) {
        prev_contour = contour;
        continue;
      }

      name = contour->d_name;

      prev_pnt = dtoc( prev_contour->x, prev_contour->y );
      cur_pnt = dtoc( contour->x, contour->y );

      printf("## contour name %i, pnt (%lli,%lli)\n",
          name,
          (long long int)cur_pnt.X,
          (long long int)cur_pnt.Y);

      if (gAperture.find(name) == gAperture.end()) {
        fprintf(stderr, "## WARNING: name %i not found in aperture library. Ignoring.\n", name);
        fprintf(stdout, "## WARNING: name %i not found in aperture library. Ignoring.\n", name);
        contour = contour->next;
        continue;
      }

      // An informative example:
      // Consider drawing a line with rounded corners from position prev_contour->[xy] to contour->[xy].
      // This is done by first drawing a (linearized) circle (in general, an aperture) at prev_countour,
      // then drawing another circle at countour.  After those two are formed, take the convex hull
      // of the two (this is the ch_graham_andrew call).  Now we have a "line" with width, which is
      // really a polygon that we've constructed.
      // After all these polygoin segments have been constructed, do a big join at the end to eliminate
      // overlap.
      //
      //

      // "simple" geometry, consisting of a simple shape or aperture 'streaked' across
      // a linear motion.
      //
      if ( (cur_pnt.X != prev_pnt.X) ||
           (cur_pnt.Y != prev_pnt.Y) ) {

        point_list.clear();
        res_point.clear();

        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + prev_pnt.X,
                                            gAperture[ name ].m_path[ii][jj].Y + prev_pnt.Y  ) );
          }
        }

        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + cur_pnt.X ,
                                            gAperture[ name ].m_path[ii][jj].Y + cur_pnt.Y ) );
          }
        }

        ConvexHull( point_list, res_point );

        if (res_point.size() == 0) {
          fprintf(stderr, "# WARNING: empty polygon found for name %i, skipping\n", name);
        }
        else {

          if (!Orientation(res_point)) { ReversePath(res_point); }

          if (_expose_bit(contour->polarity, gAperture[name].m_exposure[ii])) {
            clip.AddPath( res_point, ptSubject, true );
          }
          else {
            clip.AddPath(res_point, ptClip, true);

            it_paths.clear();
            clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);

            clip.Clear();
            clip.AddPaths(it_paths, ptSubject, true);
          }

        }

        contour = contour->next;
        continue;
      }

      if ( (gAperture[name].m_type != AD_ENUM_BLOCK) &&
           (gAperture[name].m_type != AD_ENUM_MACRO) )  {
      }

      // Otherwise, we've doing a flash of either an aperture block or a
      // 'simle' aperture.
      //

      // If it's an aperture block, we've already rendered the geometry
      // and put it into `m_geom`.
      //
      if ( (gAperture[name].m_type == AD_ENUM_BLOCK) ||
           (gAperture[name].m_type == AD_ENUM_STEP_REPEAT) ) {

        if (gAperture[name].m_geom.size() < 1) {
          fprintf(stderr, "WARNING: aperture block %i empty, skipping\n", name);
          contour = contour->next;
          continue;
        }

        it_paths.clear();
        for (ii=0; ii<gAperture[name].m_geom.size(); ii++) {
          tmp_path.clear();
          for (jj=0; jj<gAperture[name].m_geom[ii].size(); jj++) {
            tmp_path.push_back( IntPoint( gAperture[ name ].m_geom[ii][jj].X + prev_pnt.X,
                                          gAperture[ name ].m_geom[ii][jj].Y + prev_pnt.Y ) );
          }
          it_paths.push_back(tmp_path);
        }

        if ( ! _expose_bit(contour->polarity, gAperture[name].m_exposure[0]) ) {

          clip.AddPaths(it_paths, ptClip, true);

          it_paths.clear();
          clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);

          clip.Clear();
        }

        clip.AddPaths( it_paths, ptSubject, true );
        contour = contour->next;
      }

      // If it's not an aperture block, it's held in the `m_path` and we
      // evaluate each in turn.
      //
      else {

        aperture_clip.Clear();
        aperture_geom.clear();
        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {

          tmp_path.clear();
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            tmp_path.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + prev_pnt.X,
                                          gAperture[ name ].m_path[ii][jj].Y + prev_pnt.Y  ) );
          }

          if (tmp_path.size() < 2) { fprintf(stdout, "## WARNING, tmp_path.size() %i\n", (int)tmp_path.size()); fflush(stdout); continue; }

          int last_idx = (int)(tmp_path.size()-1);
          if ((tmp_path[0].X != tmp_path[last_idx].X) &&
              (tmp_path[0].Y != tmp_path[last_idx].Y)) {
            fprintf(stdout, "## WARNING, tmp_path for %i is not closed!\n", name); fflush(stdout);
            tmp_path.push_back(tmp_path[0]);
          }

          if (gAperture[name].m_exposure[ii]) {
          //if (_expose_bit(contour->polarity, gAperture[name].m_exposure[ii])) {

            aperture_clip.AddPath(tmp_path, ptSubject, true);

          }
          else {
            aperture_clip.AddPath(tmp_path, ptClip, true);
            aperture_clip.Execute(ctDifference , aperture_geom, pftNonZero, pftNonZero);

            aperture_clip.Clear();
            aperture_clip.AddPaths(aperture_geom, ptSubject, true);

            aperture_geom.clear();
          }

        }
        it_paths.clear();
        aperture_clip.Execute( ctUnion, it_paths, pftNonZero, pftNonZero );

        if ( ! _expose_bit(contour->polarity) ) {
          clip.AddPaths(it_paths, ptClip, true);

          it_paths.clear();
          clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);

          clip.Clear();
        }

        clip.AddPaths( it_paths, ptSubject, true );
        contour = contour->next;

      }

    }

  }

  clip.Execute( ctUnion, result, pftNonZero, pftNonZero  );
}
*/

// construct the final polygon set (before offsetting)
//   of the joined polygons from the parsed gerber file.
//
// The aperture macros add a lot of complexity, so the basic
// idea to construct the geometry is:
//
// * if it's a 'region', add to a running `clip` of subject
//   for a union
// * for individual elements, construct a temporry `cur_clip`
//   that does an individual subject union for exposure of 1
//   and a differenc clip of exposure 0
//
int join_polygon_set_r(Paths &result, gerber_state_t *gs, int level) {
  unsigned int i, ii, jj, _i, _j;

  gerber_item_ll_t *item_nod;
  gerber_region_t *region;

  PathSet temp_pwh_vec;
  IntPoint prev_pnt, cur_pnt;

  Clipper clip;
  int n=0, name=0, _path_polarity=1;
  int polarity = 1, d_name = -1;


  Path clip_path, point_list, res_point, tmp_path;
  Paths clip_paths, clip_result,
        aperture_geom, it_paths, tmp_paths;
  Clipper aperture_clip;

  double dx, dy;

  //--

  printf("##>> join_polygon_set_r gs %p (level %i)\n", gs, level);

  for (item_nod = gs->item_head;
       item_nod;
       item_nod = item_nod->next) {

    //DEBUG
    printf("##>> join_polygon_set_r type %i (name %i, flash %i, seg %i, reg %i)\n",
        item_nod->type, item_nod->d_name,
        GERBER_FLASH, GERBER_SEGMENT, GERBER_REGION);

    polarity = gs->_root_gerber_state->polarity;
    d_name = gs->_root_gerber_state->d_state;
    printf("## polarity:%i level:%i\n", polarity, level);

    if (item_nod->type == GERBER_LP) {
      polarity = item_nod->polarity;
      gs->_root_gerber_state->polarity = polarity;

      printf("## polarity change to %i, level:%i\n", polarity, level);

      if (polarity == 0) {
        printf("## CLEAR POLARITY (LPC)\n");
      }

      continue;
    }

    else if (item_nod->type == GERBER_D10P) {
      gs->_root_gerber_state->d_state = item_nod->d_name;
      d_name = gs->_root_gerber_state->d_state;

      printf("## d change to d%i, level:%i\n", gs->_root_gerber_state->d_state, level);
      continue;
    }

    else if (item_nod->type == GERBER_REGION) {

      temp_pwh_vec.clear();
      construct_contour_region(temp_pwh_vec, item_nod->region_head);

      for (i=0; i<temp_pwh_vec.size(); i++) {
        clip.AddPaths( temp_pwh_vec[i], ptSubject, true );
      }

      continue;
    }

    else if (item_nod->type == GERBER_SEGMENT) {

      //name = item_nod->d_name;
      name = d_name;

      region = item_nod->region_head;
      if (!region) { fprintf(stderr, "error (0)"); return -1; }

      dx = region->x;
      dy = region->y;

      prev_pnt = dtoc( region->x, region->y );

      region = region->next;
      if (!region) { fprintf(stderr, "error (1)"); return -1; }

      cur_pnt = dtoc( region->x, region->y );


      //DEBUG
      printf("## name:%i, seg: (%f,%f) -> (%f,%f)\n", name, dx, dy, region->x, region->y);


      point_list.clear();
      res_point.clear();

      for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
        for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
          point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + prev_pnt.X,
                                          gAperture[ name ].m_path[ii][jj].Y + prev_pnt.Y  ) );
        }
      }

      for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
        for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
          point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + cur_pnt.X ,
                                          gAperture[ name ].m_path[ii][jj].Y + cur_pnt.Y ) );
        }
      }

      //DEBUG
      printf("##cp.0\n"); fflush(stdout);

      ConvexHull( point_list, res_point );

      if (res_point.size() == 0) {
        fprintf(stdout, "# WARNING: empty polygon found for name %i, skipping\n", name);
        fprintf(stderr, "# WARNING: empty polygon found for name %i, skipping\n", name);
      }
      else {

        if (!Orientation(res_point)) { ReversePath(res_point); }

        //if (_expose_bit(item_nod->polarity, gAperture[name].m_exposure[ii])) {
        //if (_expose_bit(item_nod->polarity, gAperture[name].m_exposure[0])) {
        if (_expose_bit(polarity, gAperture[name].m_exposure[0])) {
          clip.AddPath( res_point, ptSubject, true );
        }
        else {
          clip.AddPath(res_point, ptClip, true);

          it_paths.clear();
          //clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);
          clip.Execute(ctDifference, it_paths, pftEvenOdd, pftEvenOdd);

          clip.Clear();
          clip.AddPaths(it_paths, ptSubject, true);
        }

      }

      //DEBUG
      printf("##cp.1\n"); fflush(stdout);


    }

    else if (item_nod->type == GERBER_FLASH) {

      //name = item_nod->d_name;
      name = d_name;
      cur_pnt = dtoc( item_nod->x, item_nod->y );

      //DEBUG
      printf("#### join_polygon_set_r flash d%i, (%f,%f)\n",
          name, (float)item_nod->x, (float)item_nod->y);

      // Simple aperture
      //
      if ( gAperture.find(name) != gAperture.end() ) {

        //DEBUG
        printf("#### flashing aperture d%i\n", name);

        aperture_clip.Clear();
        aperture_geom.clear();
        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {

          tmp_path.clear();
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            tmp_path.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + cur_pnt.X,
                                          gAperture[ name ].m_path[ii][jj].Y + cur_pnt.Y  ) );
          }

          if (tmp_path.size() < 2) { fprintf(stdout, "## WARNING, tmp_path.size() %i\n", (int)tmp_path.size()); fflush(stdout); continue; }

          int last_idx = (int)(tmp_path.size()-1);
          if ((tmp_path[0].X != tmp_path[last_idx].X) &&
              (tmp_path[0].Y != tmp_path[last_idx].Y)) {
            fprintf(stdout, "## WARNING, tmp_path for %i is not closed!\n", name); fflush(stdout);
            tmp_path.push_back(tmp_path[0]);
          }

          if (gAperture[name].m_exposure[ii]) {
            aperture_clip.AddPath(tmp_path, ptSubject, true);
          }
          else {
            aperture_clip.AddPath(tmp_path, ptClip, true);
            //aperture_clip.Execute(ctDifference , aperture_geom, pftNonZero, pftNonZero);
            aperture_clip.Execute(ctDifference , aperture_geom, pftEvenOdd, pftEvenOdd);
            aperture_clip.Clear();
            aperture_clip.AddPaths(aperture_geom, ptSubject, true);
            aperture_geom.clear();
          }

        }

        it_paths.clear();
        //aperture_clip.Execute( ctUnion, it_paths, pftNonZero, pftNonZero );
        aperture_clip.Execute( ctUnion, it_paths, pftEvenOdd, pftEvenOdd );

        //if ( ! _expose_bit(item_nod->polarity) ) {
        if ( ! _expose_bit(polarity) ) {

          //DEBUG
          printf("#### aperture d%i clear polarity (%i)\n", name, polarity);


          clip.AddPaths(it_paths, ptClip, true);
          it_paths.clear();
          //clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);
          clip.Execute(ctDifference, it_paths, pftEvenOdd, pftEvenOdd);
          clip.Clear();
        }

        //DEBUG
        printf("## adding flash subject\n");
        for (ii=0; ii<it_paths.size(); ii++) {
          for (jj=0; jj<it_paths[ii].size(); jj++) {
            printf("##X %lli %lli\n",
                (long long int)it_paths[ii][jj].X,
                (long long int)it_paths[ii][jj].Y);
          }
          printf("##X\n");
        }


        clip.AddPaths( it_paths, ptSubject, true );

      }

      // Aperture blocks
      //
      else if ( gApertureBlock.find(name) != gApertureBlock.end() ) {

        it_paths.clear();
        join_polygon_set_r( it_paths, gApertureBlock[name], level+1 );

        tmp_paths.clear();

        for (ii=0; ii<it_paths.size(); ii++) {

          if (it_paths[ii].size() < 2) { fprintf(stdout, "## WARNING, tmp_path.size() %i\n", (int)tmp_path.size()); fflush(stdout); continue; }

          tmp_path.clear();
          for (jj=0; jj<it_paths[ii].size(); jj++) {

            tmp_path.push_back( IntPoint( it_paths[ii][jj].X + cur_pnt.X,
                                          it_paths[ii][jj].Y + cur_pnt.Y ) );

          }
          tmp_paths.push_back(tmp_path);

        }

        if ( ! _expose_bit(polarity) ) {

          clip.AddPaths(tmp_paths, ptClip, true);

          tmp_paths.clear();
          //clip.Execute(ctDifference, tmp_paths, pftNonZero, pftNonZero);
          clip.Execute(ctDifference, tmp_paths, pftEvenOdd, pftEvenOdd);

          clip.Clear();
        }

        clip.AddPaths( tmp_paths, ptSubject, true );
      }

      // error
      //
      else {
        fprintf(stderr, "ERROR: could not find %i in aperture list\n", name);
        fprintf(stdout, "ERROR: could not find %i in aperture list\n", name);
        return -1;
      }


    }

    else if (item_nod->type == GERBER_SR) {

      printf("##>> wip, GERBER_SR\n");

      for (jj=0; jj<item_nod->sr_y; jj++) {
        for (ii=0; ii<item_nod->sr_x; ii++) {

          dx = (double) (((double)ii) * (item_nod->sr_i));
          dy = (double) (((double)jj) * (item_nod->sr_j));

          it_paths.clear();

          //DEBUG
          join_polygon_set_r(it_paths, item_nod->step_repeat, level+1);

          //cur_pnt = dtoc( region->x, region->y );
          cur_pnt = dtoc( item_nod->x + dx, item_nod->y + dy );
          for (_i=0; _i<it_paths.size(); _i++) {
            for (_j=0;  _j<it_paths[_i].size(); _j++) {
              it_paths[_i][_j].X += cur_pnt.X;
              it_paths[_i][_j].Y += cur_pnt.Y;
            }
          }

          /*
          if ( ! _expose_bit(polarity) ) {
            clip.AddPaths(it_paths, ptClip, true);
            it_paths.clear();
            clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);
            clip.Clear();
          }
          */

          clip.AddPaths( it_paths, ptSubject, true );

        }
      }

    }

  }

  //DEBUG
  printf("##cp.x\n"); fflush(stdout);

  //clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
  clip.Execute( ctUnion, result, pftEvenOdd, pftEvenOdd );

  //DEBUG
  printf("##cp.xx\n"); fflush(stdout);

  return 0;
}

int join_polygon_set(Paths &result, gerber_state_t *gs) {
  return join_polygon_set_r(result, gs, 0);
}

