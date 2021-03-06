// -*- mode: c++; c-basic-style: "bsd"; c-basic-offset: 4; -*-
/*
 * FMS_Data/LsDirOptions.hpp
 * Copyright (C) Cátedra SAES-UMU 2010 <andres.senac@um.es>
 *
 * EMF4CPP is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EMF4CPP is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file LsDirOptions.hpp
 * \brief The LsDirOptions class
 * \author Generated file
 * \date 31/03/2011
 */

#ifndef FMS_DATA_LSDIROPTIONS_HPP
#define FMS_DATA_LSDIROPTIONS_HPP

#include <FMS_Data_forward.hpp>
#include <ecorecpp/mapping_forward.hpp>

#include <ecore_forward.hpp>
#include <ecore_forward.hpp>

#include <ecore/EObject.hpp>

/*PROTECTED REGION ID(LsDirOptions_pre) START*/
// Please, enable the protected region if you add manually written code.
// To do this, add the keyword ENABLED before START.
/*PROTECTED REGION END*/

namespace FMS_Data
{

    /**
     * \class LsDirOptions
     * \brief Implementation of the LsDirOptions class
     */
    class LsDirOptions: public virtual ::ecore::EObject

    {
    public:
        /**
         * \brief The default constructor for LsDirOptions
         */
        LsDirOptions();
        /**
         * \brief The destructor for LsDirOptions
         */
        virtual ~LsDirOptions();

        /**
         * \brief Internal method
         */
        virtual void _initialize();

        // Operations


        // Attributes
        /**
         * \brief To get the longFormat
         * \return The longFormat attribute value
         **/
        ::ecore::EBoolean isLongFormat() const;
        /**
         * \brief To set the longFormat
         * \param _longFormat The longFormat value
         **/
        void setLongFormat(::ecore::EBoolean _longFormat);

        /**
         * \brief To get the allFiles
         * \return The allFiles attribute value
         **/
        ::ecore::EBoolean isAllFiles() const;
        /**
         * \brief To set the allFiles
         * \param _allFiles The allFiles value
         **/
        void setAllFiles(::ecore::EBoolean _allFiles);

        // References


        /*PROTECTED REGION ID(LsDirOptions) START*/
        // Please, enable the protected region if you add manually written code.
        // To do this, add the keyword ENABLED before START.
        /*PROTECTED REGION END*/

        // EObjectImpl
        virtual ::ecore::EJavaObject eGet(::ecore::EInt _featureID,
                ::ecore::EBoolean _resolve);
        virtual void eSet(::ecore::EInt _featureID,
                ::ecore::EJavaObject const& _newValue);
        virtual ::ecore::EBoolean eIsSet(::ecore::EInt _featureID);
        virtual void eUnset(::ecore::EInt _featureID);
        virtual ::ecore::EClass_ptr _eClass();

        /*PROTECTED REGION ID(LsDirOptionsImpl) START*/
        // Please, enable the protected region if you add manually written code.
        // To do this, add the keyword ENABLED before START.
        /*PROTECTED REGION END*/

    protected:
        // Attributes

        ::ecore::EBoolean m_longFormat;

        ::ecore::EBoolean m_allFiles;

        // References

    };

} // FMS_Data

#endif // FMS_DATA_LSDIROPTIONS_HPP
