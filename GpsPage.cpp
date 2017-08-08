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
#include "GpsPage.hpp"

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

Page::SelectionResponse GpsPage::select(const DisplayInfo &, SelectionCause) {
	return SelectPage;
}

void GpsPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.goodfix) {
		within = di.locerr;
		sats = di.sats;
		tds << "Within " << within << 'm' << clearTo(12, 1) << "Sats " <<
		std::left << std::setw(2) << sats;
		//<< startLine;
	} else {
		within = -1;
		sats = 0;
		tds << "No fix";
		if (di.curloc.lon == 0.0) {
			tds << "\n\n\nGPS";
			return;
		} else {
			tds << "; old postion\n";
		}
	}
	lon = di.curloc.lon;
	lat - di.curloc.lat;
	tds << "Lon " << std::setprecision(7) << std::setw(12) << std::fixed <<
	lon << "\nLat " << std::setprecision(7) << std::setw(12) << std::fixed <<
	lat << "\nGPS";
}

void GpsPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.goodfix) {
		if (within == -1) {
			// redo page
			show(di, tds);
			return;
		}
		if (within != di.locerr) {
			within = di.locerr;
			tds << move(7, 1) << within << 'm' << clearTo(11, 1);
		}
		if (sats != di.sats) {
			sats = di.sats;
			tds << move(18, 1) << std::left << std::setw(2) << sats;// << startLine;
		}
		if (lon != di.curloc.lon) {
			lon = di.curloc.lon;
			tds << move(4, 2) << std::setprecision(7) << std::setw(12) <<
			std::fixed << lon;
		}
		if (lat != di.curloc.lat) {
			lat = di.curloc.lat;
			tds << move(4, 3) << std::setprecision(7) << std::setw(12) <<
			std::fixed << lat;
		}
	} else if (within != -1) {
		// redo page
		show(di, tds);
		return;
	}
	tds << move(12, 0);
}
