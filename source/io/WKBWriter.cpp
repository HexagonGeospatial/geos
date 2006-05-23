/**********************************************************************
 * $Id$
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2005-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/io/WKBWriter.h>
#include <geos/io/WKBReader.h>
#include <geos/io/WKBConstants.h>
#include <geos/io/ByteOrderValues.h>
#include <geos/util/IllegalArgumentException.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/Point.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/LineString.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/MultiPoint.h>
#include <geos/geom/MultiLineString.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/PrecisionModel.h>

#include <ostream>
#include <sstream>
#include <cassert>

#undef DEBUG_WKB_WRITER

using namespace std;
using namespace geos::geom;

namespace geos {
	namespace io { // geos.io

WKBWriter::WKBWriter(int dims, int bo):
		outputDimension(dims), byteOrder(bo), outStream(NULL)
{
	if ( dims < 2 || dims > 3 )
		throw util::IllegalArgumentException("WKB output dimension must be 2 or 3");
}

void
WKBWriter::writeHEX(const Geometry &g, ostream &os) 
{
	// setup input/output stream
	stringstream stream;
  
	// write the geometry in wkb format
	this->write(g, stream);

	// convert to HEX
	WKBReader::printHEX(stream, os);
}

void
WKBWriter::write(const Geometry &g, ostream &os) 
{
	outStream = &os;

	switch (g.getGeometryTypeId()) {
		case GEOS_POINT:
			return writePoint((Point &)g);
		case GEOS_LINESTRING:
		case GEOS_LINEARRING:
			return writeLineString((LineString &)g);
		case GEOS_POLYGON:
			return writePolygon((Polygon &)g);
		case GEOS_MULTIPOINT:
			return writeGeometryCollection(
				(GeometryCollection &)g,
				WKBConstants::wkbMultiPoint);
		case GEOS_MULTILINESTRING:
			return writeGeometryCollection(
				(GeometryCollection &)g,
				WKBConstants::wkbMultiLineString);
		case GEOS_MULTIPOLYGON:
			return writeGeometryCollection(
				(GeometryCollection &)g,
				WKBConstants::wkbMultiPolygon);
		case GEOS_GEOMETRYCOLLECTION:
			return writeGeometryCollection(
				(GeometryCollection &)g,
				WKBConstants::wkbGeometryCollection);
		default:
			assert(0); // Unknown Geometry type
	}
}

void
WKBWriter::writePoint(const Point &g) 
{
	if (g.isEmpty()) throw
		util::IllegalArgumentException("Empty Points cannot be represented in WKB");

	writeByteOrder();
	writeGeometryType(WKBConstants::wkbPoint);

	const CoordinateSequence* cs=g.getCoordinatesRO();
	assert(cs);
	writeCoordinateSequence(*cs, false);
}

void
WKBWriter::writeLineString(const LineString &g) 
{
	writeByteOrder();
	writeGeometryType(WKBConstants::wkbLineString);
	const CoordinateSequence* cs=g.getCoordinatesRO();
	assert(cs);
	writeCoordinateSequence(*cs, true);
}

void
WKBWriter::writePolygon(const Polygon &g) 
{
	writeByteOrder();
	writeGeometryType(WKBConstants::wkbPolygon);
	int nholes = g.getNumInteriorRing();
	writeInt(nholes+1);

	const LineString* ls = g.getExteriorRing();
	assert(ls);

	const CoordinateSequence* cs=ls->getCoordinatesRO();
	assert(cs);

	writeCoordinateSequence(*cs, true);
	for (int i=0; i<nholes; i++)
	{
		ls = g.getInteriorRingN(i);
		assert(ls);

		cs = ls->getCoordinatesRO();
		assert(cs);

		writeCoordinateSequence(*cs, true);
	}
}

void
WKBWriter::writeGeometryCollection(const GeometryCollection &g,
	int wkbtype) 
{
	writeByteOrder();
	writeGeometryType(wkbtype);
	int ngeoms = g.getNumGeometries();
	writeInt(ngeoms);

	assert(outStream);
	for (int i=0; i<ngeoms; i++)
	{
		const Geometry* elem = g.getGeometryN(i);
		assert(elem);

		write(*elem, *outStream);
	}
}

void
WKBWriter::writeByteOrder() 
{
	assert(outStream);
	outStream->write(reinterpret_cast<char*>(&byteOrder), 1);
}

void
WKBWriter::writeGeometryType(int typeId) 
{
	int flag3D = (outputDimension == 3) ? 0x80000000 : 0;
        int typeInt = typeId | flag3D;
	//writeInt(typeId);
	writeInt(typeInt);
}

void
WKBWriter::writeInt(int val) 
{
	ByteOrderValues::putInt(val, buf, byteOrder);
	outStream->write(reinterpret_cast<char *>(buf), 4);
	//outStream->write(reinterpret_cast<char *>(&val), 4);
}

void
WKBWriter::writeCoordinateSequence(const CoordinateSequence &cs,
	bool sized) 
{
	int size = cs.getSize();
	bool is3d=false;
	if ( cs.getDimension() > 2 && outputDimension > 2) is3d = true;

	if (sized) writeInt(size);
	for (int i=0; i<size; i++) writeCoordinate(cs, i, is3d);
}

void
WKBWriter::writeCoordinate(const CoordinateSequence &cs, int idx,
	bool is3d) 
{
#if DEBUG_WKB_WRITER
	cout<<"writeCoordinate: X:"<<cs.getX(idx)<<" Y:"<<cs.getY(idx)<<endl;
#endif
	assert(outStream);

	ByteOrderValues::putDouble(cs.getX(idx), buf, byteOrder);
	outStream->write(reinterpret_cast<char *>(buf), 8);
	ByteOrderValues::putDouble(cs.getY(idx), buf, byteOrder);
	outStream->write(reinterpret_cast<char *>(buf), 8);
	if ( is3d )
	{
		ByteOrderValues::putDouble(
			cs.getOrdinate(idx, CoordinateSequence::X),
			buf, byteOrder);
		outStream->write(reinterpret_cast<char *>(buf), 8);
	}
}


} // namespace geos.io
} // namespace geos

