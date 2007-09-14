/***************************************************************************
 *   Copyright (C) 2005-2007 by the FIFE Team                              *
 *   fife-public@lists.sourceforge.net                                     *
 *   This file is part of FIFE.                                            *
 *                                                                         *
 *   FIFE is free software; you can redistribute it and/or modify          *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA              *
 ***************************************************************************/

// Standard C++ library includes
#include <assert.h>

// 3rd party library includes

// FIFE includes
// These includes are split up in two parts, separated by one empty line
// First block: files included from the FIFE root src directory
// Second block: files included from the same folder
#include "util/fife_math.h"
#include "util/logger.h"

#include "hexgrid.h"

namespace FIFE {
	static Logger _log(LM_HEXGRID);

	static const double HEX_WIDTH = 1;
	static const double HEX_TO_EDGE = HEX_WIDTH / 2;
	static const double HEX_TO_CORNER = 0.5 / cos(M_PI / 6);
	static const double HEX_EDGE_HALF = HEX_TO_CORNER * sin(M_PI / 6);
	static const double VERTICAL_MULTIP = sqrt(HEX_WIDTH*HEX_WIDTH - HEX_TO_EDGE*HEX_TO_EDGE);
	static const double VERTICAL_MULTIP_INV = 1 / VERTICAL_MULTIP;

	HexGrid::HexGrid(): CellGrid() {
		FL_DBG(_log, "Constructing new HexGrid");
		FL_DBG(_log, LMsg("HEX_WIDTH ") << HEX_WIDTH);
		FL_DBG(_log, LMsg("HEX_TO_EDGE ") << HEX_TO_EDGE);
		FL_DBG(_log, LMsg("HEX_TO_CORNER ") << HEX_TO_CORNER);
		FL_DBG(_log, LMsg("HEX_EDGE_HALF ") << HEX_EDGE_HALF);
		FL_DBG(_log, LMsg("VERTICAL_MULTIP ") << VERTICAL_MULTIP);
	}

	HexGrid::~HexGrid() {
	}

	bool HexGrid::isAccessible(const Point& curpos, const Point& target) {
		if (curpos == target)
			return true;
		if ((curpos.x == target.x) && (curpos.y - 1 == target.y))
			return true;
		if ((curpos.x == target.x) && (curpos.y + 1 == target.y))
			return true;
		if ((curpos.x + 1 == target.x) && (curpos.y == target.y))
			return true;
		if ((curpos.x - 1 == target.x) && (curpos.y == target.y))
			return true;

		if ((curpos.x - 1 == target.x) && (curpos.y - 1 == target.y))
			return true;
		if ((curpos.x + 1 == target.x) && (curpos.y + 1 == target.y))
			return true;

		return false;
	}

	float HexGrid::getAdjacentCost(const Point& curpos, const Point& target) {
		assert(isAccessible(curpos, target));
		if (curpos == target) {
			return 0;
		}
		return 1;
	}
	
	const std::string& HexGrid::getName() const {
		static std::string hexGrid("Hex Grid");
		return hexGrid;
	}

	double HexGrid::getXZigzagOffset(double y) {
		// each uneven row has shifted coordinate of 0.5 horizontally
		// shift has to be gradual on vertical axis
		double ay = ABS(y);
		int i_layer_y = static_cast<int>(ay);
		double offset = ay - static_cast<double>(i_layer_y);
		if ((i_layer_y % 2) == 1) {
			offset = 1 - offset;
		}
		return HEX_TO_EDGE * offset;
	}
	
	DoublePoint HexGrid::toElevationCoordinates(const DoublePoint& layer_coords) {
		DoublePoint tranformed_coords(layer_coords);
		tranformed_coords.x += getXZigzagOffset(layer_coords.y);
		tranformed_coords.y *= VERTICAL_MULTIP;
		DoublePoint result = m_matrix * tranformed_coords;
		FL_DBG(_log, LMsg("layercoords ") << layer_coords << " converted to elev: " << result);
		return result;
	}

	DoublePoint HexGrid::toExactLayerCoordinates(const DoublePoint& elevation_coord) {
		DoublePoint layer_coords = m_inverse_matrix * elevation_coord;
		layer_coords.y /= VERTICAL_MULTIP;
		layer_coords.x -= getXZigzagOffset(layer_coords.y);
		FL_DBG(_log, LMsg("elevcoords ") << elevation_coord << " converted to layer: " << layer_coords);
		return layer_coords;
	}

	Point HexGrid::toLayerCoordinates(const DoublePoint& elevation_coord) {
		FL_DBG(_log, LMsg("==============\nConverting elev coords ") << elevation_coord << " to int layer coords...");
		DoublePoint elc = m_inverse_matrix * elevation_coord;
		elc.y *= VERTICAL_MULTIP_INV;
		DoublePoint lc = DoublePoint(floor(elc.x), floor(elc.y));
		double dx = elc.x - lc.x;
		double dy = elc.y - lc.y;
		int x = static_cast<int>(lc.x);
		int y = static_cast<int>(lc.y);
		FL_DBG(_log, LMsg("elc=") << elc << ", lc=" << lc);
		FL_DBG(_log, LMsg("x=") << x << ", y=" << y << ", dx=" << dx << ", dy=" << dy);
		Point result;
		
		if ((y % 2) == 0) {
			FL_DBG(_log, "In even row");
			if ((1 - dy) < HEX_EDGE_HALF) {
				FL_DBG(_log, "In lower rect area");
				result = Point(x, y+1);
			} 
			else if (dy < HEX_EDGE_HALF) {
				FL_DBG(_log, "In upper rect area");
				if (dx > 0.5) {
					FL_DBG(_log, "...on right");
					result = Point(x+1, y);
				} 
				else {
					FL_DBG(_log, "...on left");
					result = Point(x, y);
				}
			} 
			// in middle triangle area
			else {
				FL_DBG(_log, "In middle triangle area");
				if (dx < 0.5) {
					FL_DBG(_log, "In left triangles");
					if (ptInTriangle(DoublePoint(dx, dy),
					                 DoublePoint(0, VERTICAL_MULTIP * HEX_EDGE_HALF), 
					                 DoublePoint(0, VERTICAL_MULTIP * (1-HEX_EDGE_HALF)), 
					                 DoublePoint(0.5, VERTICAL_MULTIP * HEX_EDGE_HALF)
					                 )) {
						FL_DBG(_log, "..upper part");
						result = Point(x, y);
					} else {
						FL_DBG(_log, "..lower part");
						result = Point(x, y+1);
					}
				} else {
					FL_DBG(_log, "In right triangles");
					if (ptInTriangle(DoublePoint(dx, dy),
					                 DoublePoint(1, VERTICAL_MULTIP * HEX_EDGE_HALF), 
					                 DoublePoint(1, VERTICAL_MULTIP * (1-HEX_EDGE_HALF)), 
					                 DoublePoint(0.5, VERTICAL_MULTIP * HEX_EDGE_HALF)
					                 )) {
						FL_DBG(_log, "..upper part");
						result = Point(x+1, y);
					} else {
						FL_DBG(_log, "..lower part");
						result = Point(x, y+1);
					}
				}
			}		
		} 
		else {
			FL_DBG(_log, "In uneven row");
			if (dy < HEX_EDGE_HALF) {
				FL_DBG(_log, "In upper rect area");
				result = Point(x, y);
			} 
			else if ((1 - dy) < HEX_EDGE_HALF) {
				FL_DBG(_log, "In lower rect area");
				if (dx > 0.5) {
					FL_DBG(_log, "...on right");
					result = Point(x+1, y+1);
				} 
				else {
					FL_DBG(_log, "...on left");
					result = Point(x, y+1);
				}
			} 
			else {
				FL_DBG(_log, "In middle triangle area");
				if (dx < 0.5) {
					FL_DBG(_log, "In left triangles");
					if (ptInTriangle(DoublePoint(dx, dy),
					                 DoublePoint(0, VERTICAL_MULTIP * HEX_EDGE_HALF), 
					                 DoublePoint(0, VERTICAL_MULTIP * (1-HEX_EDGE_HALF)),
					                 DoublePoint(0.5, VERTICAL_MULTIP * (1-HEX_EDGE_HALF))
					                 )) {
						FL_DBG(_log, "..lower part");
						result = Point(x, y+1);
					} else {
						FL_DBG(_log, "..upper part");
						result = Point(x, y);
					}
				} else {
					FL_DBG(_log, "In right triangles");
					if (ptInTriangle(DoublePoint(dx, dy),
					                 DoublePoint(1, VERTICAL_MULTIP * HEX_EDGE_HALF), 
					                 DoublePoint(1, VERTICAL_MULTIP * (1-HEX_EDGE_HALF)), 
					                 DoublePoint(0.5, VERTICAL_MULTIP * (1-HEX_EDGE_HALF))
					                 )) {
					        FL_DBG(_log, "..lower part");
						result = Point(x+1, y+1);
					} else {
						FL_DBG(_log, "..upper part");
						result = Point(x, y);
					}
				}
			}
		}
		FL_DBG(_log, LMsg("  result = ") << result);
		return result;
	}

	void HexGrid::getVertices(std::vector<DoublePoint>& vtx, const Point& cell) {
		FL_DBG(_log, LMsg("===============\ngetting vertices for ") << cell);
		vtx.clear();
		double x = static_cast<double>(cell.x);
		double y = static_cast<double>(cell.y);
		double horiz_shift = 0;
		if (cell.y % 2 != 0) {
			horiz_shift = HEX_TO_EDGE;
			FL_DBG(_log, "on uneven row");
		}
		double tx, ty;
		
		#define ADD_PT(_x, _y) vtx.push_back(DoublePoint(_x, _y));
		// FL_DBG(_log, LMsg("Added point ") << _x << ", " << _y)
		ty = y - VERTICAL_MULTIP_INV * HEX_EDGE_HALF;
		tx = x - HEX_TO_EDGE - getXZigzagOffset(ty) + horiz_shift;
		ADD_PT(tx, ty);
		
		ty = y - VERTICAL_MULTIP_INV * HEX_TO_CORNER;
		tx = x - getXZigzagOffset(ty) + horiz_shift;
		ADD_PT(tx, ty);
		
		ty = y - VERTICAL_MULTIP_INV * HEX_EDGE_HALF;
		tx = x + HEX_TO_EDGE - getXZigzagOffset(ty) + horiz_shift;
		ADD_PT(tx, ty);
		
		ty = y + VERTICAL_MULTIP_INV * HEX_EDGE_HALF;
		tx = x + HEX_TO_EDGE - getXZigzagOffset(ty) + horiz_shift;
		ADD_PT(tx, ty);
		
		ty = y + VERTICAL_MULTIP_INV * HEX_TO_CORNER;
		tx = x - getXZigzagOffset(ty) + horiz_shift;
		ADD_PT(tx, ty);
		
		ty = y + VERTICAL_MULTIP_INV * HEX_EDGE_HALF;
		tx = x - HEX_TO_EDGE - getXZigzagOffset(ty) + horiz_shift;
		ADD_PT(tx, ty);
	}
}