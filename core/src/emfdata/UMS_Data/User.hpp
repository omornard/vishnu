// -*- mode: c++; c-basic-style: "bsd"; c-basic-offset: 4; -*-
/*
 * UMS_Data/User.hpp
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
 * \file User.hpp
 * \brief The User class
 * \author Generated file
 * \date 31/03/2011
 */

#ifndef UMS_DATA_USER_HPP
#define UMS_DATA_USER_HPP

#include <UMS_Data_forward.hpp>
#include <ecorecpp/mapping_forward.hpp>

#include <ecore_forward.hpp>

#include <ecore/EObject.hpp>

/*PROTECTED REGION ID(User_pre) START*/
// Please, enable the protected region if you add manually written code.
// To do this, add the keyword ENABLED before START.
/*PROTECTED REGION END*/

namespace UMS_Data
{

    /**
     * \class User
     * \brief Implementation of the User class
     */
    class User: public virtual ::ecore::EObject

    {
    public:
        /**
         * \brief The default constructor for User
         */
        User();
        /**
         * \brief The destructor for User
         */
        virtual ~User();

        /**
         * \brief Internal method
         */
        virtual void _initialize();

        // Operations


        // Attributes
        /**
         * \brief To get the userId
         * \return The userId attribute value
         **/
        ::ecore::EString const& getUserId() const;
        /**
         * \brief To set the userId
         * \param _userId The userId value
         **/
        void setUserId(::ecore::EString const& _userId);

        /**
         * \brief To get the password
         * \return The password attribute value
         **/
        ::ecore::EString const& getPassword() const;
        /**
         * \brief To set the password
         * \param _password The password value
         **/
        void setPassword(::ecore::EString const& _password);

        /**
         * \brief To get the firstname
         * \return The firstname attribute value
         **/
        ::ecore::EString const& getFirstname() const;
        /**
         * \brief To set the firstname
         * \param _firstname The firstname value
         **/
        void setFirstname(::ecore::EString const& _firstname);

        /**
         * \brief To get the lastname
         * \return The lastname attribute value
         **/
        ::ecore::EString const& getLastname() const;
        /**
         * \brief To set the lastname
         * \param _lastname The lastname value
         **/
        void setLastname(::ecore::EString const& _lastname);

        /**
         * \brief To get the privilege
         * \return The privilege attribute value
         **/
        ::UMS_Data::PrivilegeType getPrivilege() const;
        /**
         * \brief To set the privilege
         * \param _privilege The privilege value
         **/
        void setPrivilege(::UMS_Data::PrivilegeType _privilege);

        /**
         * \brief To get the email
         * \return The email attribute value
         **/
        ::ecore::EString const& getEmail() const;
        /**
         * \brief To set the email
         * \param _email The email value
         **/
        void setEmail(::ecore::EString const& _email);

        /**
         * \brief To get the status
         * \return The status attribute value
         **/
        ::UMS_Data::StatusType getStatus() const;
        /**
         * \brief To set the status
         * \param _status The status value
         **/
        void setStatus(::UMS_Data::StatusType _status);

        // References


        /*PROTECTED REGION ID(User) START*/
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

        /*PROTECTED REGION ID(UserImpl) START*/
        // Please, enable the protected region if you add manually written code.
        // To do this, add the keyword ENABLED before START.
        /*PROTECTED REGION END*/

    protected:
        // Attributes

        ::ecore::EString m_userId;

        ::ecore::EString m_password;

        ::ecore::EString m_firstname;

        ::ecore::EString m_lastname;

        ::UMS_Data::PrivilegeType m_privilege;

        ::ecore::EString m_email;

        ::UMS_Data::StatusType m_status;

        // References

    };

} // UMS_Data

#endif // UMS_DATA_USER_HPP
