/*
 * This file is part of the Eclipse2017 project. It is subject to the GPLv3
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at
 * https://github.com/jjackowski/eclipse2017/blob/master/LICENSE.
 * No part of the Eclipse2017 project, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include "Umbra.hpp"
#include <boost/exception/errinfo_file_name.hpp>

Umbra::Umbra(const std::string &fname) : first(-1), last(-1) {
	GDALAllRegister();
	dataset = GDALDatasetUPtr((GDALDataset*)GDALOpenEx(
		fname.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr
	), GDALDatasetDeleter());
	if (!dataset) {
		BOOST_THROW_EXCEPTION(UmbraOpenError() << boost::errinfo_file_name(fname));
	}
	umbras = dataset->GetLayerByName("umbra17_1s");
	if (!umbras) {
		BOOST_THROW_EXCEPTION(UmbraNoLayer() << boost::errinfo_file_name(fname)
			<< UmbraLayerName("umbra17_1s")
		);
	}
	featdef = umbras->GetLayerDefn();
	total = umbras->GetFeatureCount();
	//aug21 = boost::gregorian::date(2017, boost::gregorian::Aug, 21)
}

bool Umbra::check(double lon, double lat) {
	OGRPoint loc(lon, lat);
	umbras->ResetReading();
	OGRFeatureUPtr feature;
	GIntBig start;
	// found an intersection earlier?
	if (first > 0) {
		// This is an imperfect and messy attempt at optimizing the search
		// without going in reverse while trying to assure a correct answer.
		// It is much less effective than location comparisons using just the
		// center of the shadow in the loop below.
		// Attempt to use previous location to limit search; offset search
		// to the west because it moves to the east. Positive is eastward.
		// Up to 161 shadow shapes may overlap. Approx 130 per degree.
		double diff = (lon - poslon);
		if (diff < 0) {
			diff *= 138.0;  // some extra past 130
		} else {
			diff *= 34;  // much smaller eastward change to avoid overshoot
		}
		// subtract by litteral for overlapping shadows
		start = first - 162 + (GIntBig)diff;
		// keep start within the bounds of the data
		if (start < 0) {
			start = 0;
		} else if (start >= (total - 256)) {
			// minus 256 to provide some range to the search
			start = total - 256;
		}
		feature = OGRFeatureUPtr(umbras->GetFeature(start), OGRFeatureDeleter());
	} else {
		start = 0;
		// start from the begining; could optimize it by guessing a spot
		// part-way through the shapes
		feature = OGRFeatureUPtr(umbras->GetNextFeature(), OGRFeatureDeleter());
	}
	if (!feature) {
		BOOST_THROW_EXCEPTION(UmbraNoFeature() << UmbraFeatureIndex(start));
	}
	int  missed = 0;
	bool foundFirst = false;
	do {
		// check rough location first; much faster than checking agains polygon
		double flon = feature->GetFieldAsDouble(2);
		if (abs(loc.getX() - flon) > 0.8) { // not sure how good 0.8 is
			if (foundFirst) break;
			continue;
		}
		double flat = feature->GetFieldAsDouble(3);
		if (abs(loc.getY() - flat) > 0.8) {
			if (foundFirst) break;
			continue;
		}
		// test the location against the umbra's shape
		OGRGeometry *shadow = feature->GetGeometryRef();
		if (shadow) { // should always be true
			if (loc.Within(shadow)) {
				if (foundFirst) {
					last = feature->GetFID();
					endT = feature->GetFieldAsInteger(1);
					missed = 0;
				} else {
					first = feature->GetFID();
					startT = feature->GetFieldAsInteger(1);
					foundFirst = true;
				}
			} else if (foundFirst) {
				++missed;
			}
		}
	} while ((missed < 3) && (feature = OGRFeatureUPtr(
		umbras->GetNextFeature(), OGRFeatureDeleter()
	)));
	// didn't find shapes with the point, but did before?
	if (!foundFirst && (first >= 0)) {
		// try again
		first = last = -1;
		return check(lon, lat);
	}
	return foundFirst;
}
