/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (c) 2024 Martin Davis
 * Copyright (C) 2024 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/algorithm/BoundaryNodeRule.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/Dimension.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryCollection.h>
#include <geos/geom/LineString.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/Location.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/Point.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/util/ComponentCoordinateExtracter.h>
#include <geos/geom/util/PointExtracter.h>
#include <geos/operation/relateng/RelateGeometry.h>
#include <geos/operation/relateng/RelateSegmentString.h>
#include <geos/operation/relateng/DimensionLocation.h>


#include <sstream>


using geos::algorithm::BoundaryNodeRule;
using namespace geos::geom;


namespace geos {      // geos
namespace operation { // geos.operation
namespace relateng {  // geos.operation.relateng


RelateGeometry::RelateGeometry(const Geometry* input, bool isPrepared, const BoundaryNodeRule& bnRule)
    : geom(input)
    , m_isPrepared(isPrepared)
    , geomEnv(input->getEnvelopeInternal())
    , boundaryNodeRule(bnRule)
    , geomDim(input->getDimension())
    , isLineZeroLen(isZeroLength(input))
    , isGeomEmpty(input->isEmpty())
{
    analyzeDimensions();
}


/* public static */
std::string
RelateGeometry::name(bool isA)
{
    return isA ? "A" : "B";
}


/* private */
void
RelateGeometry::analyzeDimensions()
{
    if (isGeomEmpty) {
        return;
    }
    GeometryTypeId typeId = geom->getGeometryTypeId();
    if (typeId == GEOS_POINT || typeId == GEOS_MULTIPOINT) {
        hasPoints = true;
        geomDim = Dimension::P;
        return;
    }
    if (typeId == GEOS_LINESTRING || typeId == GEOS_MULTILINESTRING) {
        hasLines = true;
        geomDim = Dimension::L;
        return;
    }
    if (typeId == GEOS_POLYGON || typeId == GEOS_MULTIPOLYGON) {
        hasAreas = true;
        geomDim = Dimension::A;
        return;
    }
    //-- analyze a (possibly mixed type) collection
    std::vector<const Geometry*> elems;
    const GeometryCollection* col = static_cast<const GeometryCollection*>(geom);
    col->getAllGeometries(elems);
    for (const Geometry* elem : elems)
    {
        if (elem->isEmpty())
            continue;
        if (elem->getGeometryTypeId() == GEOS_POINT) {
            hasPoints = true;
            if (geomDim < Dimension::P) geomDim = Dimension::P;
        }
        if (elem->getGeometryTypeId() == GEOS_LINESTRING) {
            hasLines = true;
            if (geomDim < Dimension::L) geomDim = Dimension::L;
        }
        if (elem->getGeometryTypeId() == GEOS_POLYGON) {
            hasAreas = true;
            if (geomDim < Dimension::A) geomDim = Dimension::A;
        }
    }
}


/* private static */
bool
RelateGeometry::isZeroLength(const Geometry* geom)
{
    std::vector<const Geometry*> elems;
    const GeometryCollection* col = static_cast<const GeometryCollection*>(geom);
    col->getAllGeometries(elems);
    for (const Geometry* elem : elems) {
        if (elem->getGeometryTypeId() == GEOS_LINESTRING) {
            if (! isZeroLength(static_cast<const LineString*>(elem))) {
                return false;
            }
        }
    }
    return true;
}

/* private static */
bool
RelateGeometry::isZeroLength(const LineString* line) {
    if (line->getNumPoints() >= 2) {
        const CoordinateXY& p0 = line->getCoordinateN(0);
        for (std::size_t i = 1; i < line->getNumPoints(); i++) {
            // NOTE !!! CHANGE FROM JTS, original below
            // const CoordinateXY& pi = line.getCoordinateN(1);
            const CoordinateXY& pi = line->getCoordinateN(i);
            //-- most non-zero-len lines will trigger this right away
            if (! p0.equals2D(pi)) {
                return false;
            }
        }
    }
    return true;
}


/* public */
const Geometry*
RelateGeometry::getGeometry() const
{
    return geom;
}

/* public */
bool
RelateGeometry::isPrepared() const
{
    return m_isPrepared;
}

/* public */
const Envelope*
RelateGeometry::getEnvelope() const
{
    return geomEnv;
}

/* public */
int
RelateGeometry::getDimension() const
{
    return geomDim;
}

/* public */
bool
RelateGeometry::hasDimension(int dim) const
{
    switch (dim) {
        case Dimension::P: return hasPoints;
        case Dimension::L: return hasLines;
        case Dimension::A: return hasAreas;
    }
    return false;
}


/* public */
int
RelateGeometry::getDimensionReal() const
{
    if (isGeomEmpty)
        return Dimension::False;
    if (getDimension() == Dimension::L && isLineZeroLen)
        return Dimension::P;
    if (hasAreas)
        return Dimension::A;
    if (hasLines)
        return Dimension::L;
    return Dimension::P;
}

/* public */
bool
RelateGeometry::hasEdges() const
{
    return hasLines || hasAreas;
}

/* private */
RelatePointLocator*
RelateGeometry::getLocator()
{
    if (locator == nullptr)
        locator.reset(new RelatePointLocator(geom, m_isPrepared, boundaryNodeRule));
    return locator.get();
}


/* public */
bool
RelateGeometry::isNodeInArea(const CoordinateXY* nodePt, const Geometry* parentPolygonal)
{
    int dimLoc = getLocator()->locateNodeWithDim(nodePt, parentPolygonal);
    return dimLoc == DimensionLocation::AREA_INTERIOR;
}


/* public */
Location
RelateGeometry::locateLineEnd(const CoordinateXY* p)
{
    return getLocator()->locateLineEnd(p);
}


/* public */
Location
RelateGeometry::locateAreaVertex(const CoordinateXY* pt)
{
    /**
     * Can pass a null polygon, because the point is an exact vertex,
     * which will be detected as being on the boundary of its polygon
     */
    return locateNode(pt, nullptr);
}


/* public */
Location
RelateGeometry::locateNode(const CoordinateXY* pt, const Geometry* parentPolygonal)
{
    return getLocator()->locateNode(pt, parentPolygonal);
}


/* public */
int
RelateGeometry::locateWithDim(const CoordinateXY* pt)
{
    int loc = getLocator()->locateWithDim(pt);
    return loc;
}


/* public */
bool
RelateGeometry::isPointsOrPolygons() const
{
    GeometryTypeId typeId = geom->getGeometryTypeId();
    return typeId == GEOS_POINT
        || typeId == GEOS_MULTIPOINT
        || typeId == GEOS_POLYGON
        || typeId == GEOS_MULTIPOLYGON;
}


/* public */
bool
RelateGeometry::isPolygonal() const
{
    //TODO: also true for a GC containing one polygonal element (and possibly some lower-dimension elements)
    GeometryTypeId typeId = geom->getGeometryTypeId();
    return typeId == GEOS_POLYGON
        || typeId == GEOS_MULTIPOLYGON;
}


/* public */
bool
RelateGeometry::isEmpty() const
{
    return isGeomEmpty;
}


/* public */
bool
RelateGeometry::hasBoundary()
{
    return getLocator()->hasBoundary();
}


/* public */
Coordinate::ConstXYSet&
RelateGeometry::getUniquePoints()
{
    if (uniquePoints.empty()) {
        uniquePoints = createUniquePoints();
    }
    return uniquePoints;
}


/* private */
Coordinate::ConstXYSet
RelateGeometry::createUniquePoints()
{
    //-- only called on P geometries
    std::vector<const CoordinateXY*> pts;
    geom::util::ComponentCoordinateExtracter::getCoordinates(*geom, pts);
    Coordinate::ConstXYSet set(pts.begin(), pts.end());
    return set;
}


/* public */
std::vector<const Point*>
RelateGeometry::getEffectivePoints()
{
    std::vector<const Point*> ptListAll;
    geom::util::PointExtracter::getPoints(*geom, ptListAll);

    if (getDimensionReal() <= Dimension::P)
        return ptListAll;

    //-- only return Points not covered by another element
    std::vector<const Point*> ptList;
    for (const Point* p : ptListAll) {
        int locDim = locateWithDim(p->getCoordinate());
        if (DimensionLocation::dimension(locDim) == Dimension::P) {
            ptList.push_back(p);
        }
    }
    return ptList;
}


/* public */
std::vector<std::unique_ptr<RelateSegmentString>>
RelateGeometry::extractSegmentStrings(bool isA, const Envelope* env)
{
    std::vector<std::unique_ptr<RelateSegmentString>> segStrings;
    extractSegmentStrings(isA, env, geom, segStrings);
    return segStrings;
}


/* private */
void
RelateGeometry::extractSegmentStrings(bool isA,
    const Envelope* env, const Geometry* p_geom,
    std::vector<std::unique_ptr<RelateSegmentString>>& segStrings)
{
    //-- record if parent is MultiPolygon
    const MultiPolygon* parentPolygonal = nullptr;
    if (p_geom->getGeometryTypeId() == GEOS_MULTIPOLYGON) {
        parentPolygonal = static_cast<const MultiPolygon*>(p_geom);
    }

    for (std::size_t i = 0; i < p_geom->getNumGeometries(); i++) {
        const Geometry* g = p_geom->getGeometryN(i);
        if (g->getGeometryTypeId() == GEOS_GEOMETRYCOLLECTION) {
            extractSegmentStrings(isA, env, g, segStrings);
        }
        else {
            extractSegmentStringsFromAtomic(isA, g, parentPolygonal, env, segStrings);
        }
    }
}


/* private */
void
RelateGeometry::extractSegmentStringsFromAtomic(bool isA,
    const Geometry* p_geom, const MultiPolygon* parentPolygonal,
    const Envelope* env,
    std::vector<std::unique_ptr<RelateSegmentString>>& segStrings)
{
    if (p_geom->isEmpty())
        return;

    bool doExtract = (env == nullptr) || env->intersects(p_geom->getEnvelopeInternal());
    if (! doExtract)
        return;

    elementId++;
    if (p_geom->getGeometryTypeId() == GEOS_LINESTRING) {
        const LineString* line = static_cast<const LineString*>(p_geom);
        auto cs = line->getCoordinatesRO();
        auto ss = RelateSegmentString::createLine(cs, isA, elementId, this);
        segStrings.emplace_back(ss.release());
    }
    else if (p_geom->getGeometryTypeId() == GEOS_POLYGON) {
        const Polygon* poly = static_cast<const Polygon*>(p_geom);
        const Geometry* parentPoly;
        if (parentPolygonal != nullptr)
            parentPoly = static_cast<const Geometry*>(parentPolygonal);
        else
            parentPoly = static_cast<const Geometry*>(poly);
        extractRingToSegmentString(isA, poly->getExteriorRing(), 0, env, parentPoly, segStrings);
        for (uint32_t i = 0; i < poly->getNumInteriorRing(); i++) {
            extractRingToSegmentString(isA, poly->getInteriorRingN(i), static_cast<int>(i+1), env, parentPoly, segStrings);
        }
    }
}


/* private */
void
RelateGeometry::extractRingToSegmentString(bool isA,
    const LinearRing* ring, int ringId, const Envelope* env,
    const Geometry* parentPoly,
    std::vector<std::unique_ptr<RelateSegmentString>>& segStrings)
{
    if (ring->isEmpty())
        return;
    if (env != nullptr && ! env->intersects(ring->getEnvelopeInternal()))
        return;

    const CoordinateSequence* pts = ring->getCoordinatesRO();
    auto ss = RelateSegmentString::createRing(pts, isA, elementId, ringId, parentPoly, this);
    segStrings.emplace_back(ss.release());
}


/* public */
std::string
RelateGeometry::toString() const
{
    return geom->toString();
}


/* public friend */
std::ostream&
operator<<(std::ostream& os, const RelateGeometry& rg)
{
    os << rg.toString();
    return os;
}



} // namespace geos.operation.overlayng
} // namespace geos.operation
} // namespace geos


