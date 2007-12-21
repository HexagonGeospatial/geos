/**********************************************************************
 * $Id
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 *
 **********************************************************************/

#ifndef GEOS_GEOM_PREP_PREPAREDGEOMETRYFACTORY_H
#define GEOS_GEOM_PREP_PREPAREDGEOMETRYFACTORY_H

namespace geos {
	namespace geom {
		class Geometry;

		namespace prep {
			class PreparedGeometry;
		}
	}
}

using namespace geos::geom;

namespace geos {
namespace geom { // geos::geom
namespace prep { // geos::geom::prep


/**
 * A factory for creating {@link PreparedGeometry}s.
 * It chooses an appropriate implementation of PreparedGeometry
 * based on the geoemtric type of the input geometry.
 * <p>
 * In the future, the factory may accept hints that indicate
 * special optimizations which can be performed.
 * 
 * 
 * @author Martin Davis
 *
 */
class PreparedGeometryFactory
{
private:
protected:
public:
	/**
	* Creates a new {@link PreparedGeometry} appropriate for the argument {@link Geometry}.
	* 
	* @param geom the geometry to prepare
	* @return the prepared geometry
	*/
	static const PreparedGeometry * prepare( const geom::Geometry * geom) 
	{
		PreparedGeometryFactory pf;
		return pf.create(geom); 
	}

	PreparedGeometryFactory() 
	{ }

	/**
 	* Creates a new {@link PreparedGeometry} appropriate for the argument {@link Geometry}.
 	* 
	* @param geom the geometry to prepare
	* @return the prepared geometry
	*/
	const PreparedGeometry * PreparedGeometryFactory::create( const geom::Geometry * geom) const;
};

} // namespace geos::geom::prep
} // namespace geos::geom
} // namespace geos

#endif // GEOS_GEOM_PREP_PREPAREDGEOMETRYFACTORY_H
/**********************************************************************
 * $Log$
 **********************************************************************/

