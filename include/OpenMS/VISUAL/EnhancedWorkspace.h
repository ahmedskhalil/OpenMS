// -*- mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//
// --------------------------------------------------------------------------
//                   OpenMS Mass Spectrometry Framework 
// --------------------------------------------------------------------------
//  Copyright (C) 2003-2011 -- Oliver Kohlbacher, Knut Reinert
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Marc Sturm $
// --------------------------------------------------------------------------

#ifndef OPENMS_VISUAL_ENHANCEDWORKSPACE_H
#define OPENMS_VISUAL_ENHANCEDWORKSPACE_H

#include <OpenMS/config.h>

#include <QtGui/QWorkspace>

class QMimeData;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace OpenMS
{
	class OPENMS_GUI_DLLAPI EnhancedWorkspace
		: public QWorkspace
	{
		Q_OBJECT
		
		public:
			
			/// Constructor
			EnhancedWorkspace(QWidget* parent);
			
			/// Destructor
			virtual ~EnhancedWorkspace();
		
		signals:
			
			/// Signal that is emitted, when a drag-and-drop action ends on this widget
			void dropReceived(const QMimeData* data, QWidget* source, int id);
		
		protected:
			
			///@name reimplemented Qt events
      //@{
     	void dragEnterEvent(QDragEnterEvent* event);
			void dragMoveEvent(QDragMoveEvent* event);
			void dropEvent(QDropEvent* event);
			//@}
	};
}

#endif
