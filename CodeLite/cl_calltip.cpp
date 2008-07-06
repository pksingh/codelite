//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah
// file name            : cl_calltip.cpp
//
// -------------------------------------------------------------------------
// A
//              _____           _      _     _ _
//             /  __ \         | |    | |   (_) |
//             | /  \/ ___   __| | ___| |    _| |_ ___
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/
//              \____/\___/ \__,_|\___\_____/_|\__\___|
//
//                                                  F i l e
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#include "precompiled_header.h"
#include <map>
#include "ctags_manager.h"
#include "cl_calltip.h"

#ifdef __VISUALC__
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#endif

struct tagCallTipInfo {
	wxString sig;
	wxString retValue;
};

clCallTip::clCallTip(const std::vector<TagEntryPtr> &tips)
		: m_tips(tips)
		, m_curr(0)
{
}

clCallTip::clCallTip(const clCallTip& rhs)
{
	*this = rhs;
}

clCallTip& clCallTip::operator =(const clCallTip& rhs)
{
	if (this == &rhs)
		return *this;
	m_tips = rhs.m_tips;
	return *this;
}

wxString clCallTip::First()
{
	m_curr = 0;
	if (m_tips.empty())
		return wxEmptyString;
	return TipAt(0);
}

wxString clCallTip::TipAt(int at)
{
	wxString tip;
	if ( m_tips.size() > 1 )
		tip << _T("\n\001 ") << static_cast<int>(m_curr)+1 << _T(" of ") << static_cast<int>(m_tips.size()) << _T(" \002 ")
		<< m_tips.at(at) << _T("\n");
	else
		tip << _T("\n") << m_tips.at( 0 ) << _T("\n");
	return tip;
}

wxString clCallTip::Next()
{
	// format a tip string and return it
	wxString tip;
	if ( m_tips.empty() )
		return wxEmptyString;

	m_curr++;
	if ( m_curr >= (int)m_tips.size() ) {
		m_curr = 0;
	} // if( m_curr >= m_tips.size() )

	return TipAt(m_curr);
}

wxString clCallTip::Prev()
{
	// format a tip string and return it
	wxString tip;
	if ( m_tips.empty() )
		return wxEmptyString;

	m_curr--;
	if (m_curr < 0) {
		m_curr = (int)m_tips.size()-1;
	}
	return TipAt(m_curr);
}

int clCallTip::Count() const
{
	return (int)m_tips.size();
}

wxString clCallTip::All()
{
	wxString tip;
	std::map<wxString, tagCallTipInfo> mymap;
	for (size_t i=0; i< m_tips.size(); i++) {
		tagCallTipInfo cti;
		TagEntryPtr t = m_tips.at(i);
		if (t->GetKind() == wxT("function") || t->GetKind() == wxT("prototype")) {

			wxString raw_sig ( t->GetSignature().Trim().Trim(false) );

			// evaluate the return value of the tag
			clFunction foo;
			if (LanguageST::Get()->FunctionFromPattern(t->GetPattern(), foo)) {
				if (foo.m_retrunValusConst.empty() == false) {
					cti.retValue << _U(foo.m_retrunValusConst.c_str()) << wxT(" ");
				}

				if (foo.m_returnValue.m_typeScope.empty() == false) {
					cti.retValue << _U(foo.m_returnValue.m_typeScope.c_str()) << wxT("::");
				}

				if (foo.m_returnValue.m_type.empty() == false) {
					cti.retValue << _U(foo.m_returnValue.m_type.c_str());
					if (foo.m_returnValue.m_templateDecl.empty() == false) {
						cti.retValue << wxT("<") << _U(foo.m_returnValue.m_templateDecl.c_str()) << wxT(">");
					}
					cti.retValue << _U(foo.m_returnValue.m_starAmp.c_str());
					cti.retValue << wxT(" ");
				}
			}

			bool hasDefaultValues = (raw_sig.Find(wxT("=")) != wxNOT_FOUND);
			wxString  normalizedSig = TagsManagerST::Get()->NormalizeFunctionSig(raw_sig);
			cti.sig = normalizedSig;

			if (hasDefaultValues) {
				//incase default values exist in this prototype,
				//make it the tip instead of the existing one
				cti.sig = raw_sig;
				mymap[normalizedSig] = cti;
			}

			//make sure we dont add duplicates
			if ( mymap.find(normalizedSig) == mymap.end() ) {
				//add it
				cti.sig = TagsManagerST::Get()->NormalizeFunctionSig(raw_sig, true);
				mymap[normalizedSig] = cti;
			}

		} else {
			// macro
			wxString macroName = t->GetName();
			wxString pattern = t->GetPattern();

			int where = pattern.Find(macroName);
			if (where != wxNOT_FOUND) {
				//remove the #define <name> from the pattern
				pattern = pattern.Mid(where + macroName.Length());
				pattern = pattern.Trim().Trim(false);
				if (pattern.StartsWith(wxT("("))) {
					//this macro has the form of a function
					pattern = pattern.BeforeFirst(wxT(')'));
					pattern.Append(wxT(')'));
					cti.sig = pattern.Trim().Trim(false);
					mymap[cti.sig] = cti;
				}
			}
		}
	}

	std::map<wxString, tagCallTipInfo>::iterator iter = mymap.begin();
	for ( ; iter != mymap.end(); iter++ ) {
		if( iter->second.retValue.empty() == false ) {
			tip <<  iter->second.retValue.Trim(false).Trim() << wxT(" : ");
		}
		tip << iter->second.sig << wxT("\n");
	}
	tip = tip.BeforeLast(wxT('\n'));
	return tip;
}
