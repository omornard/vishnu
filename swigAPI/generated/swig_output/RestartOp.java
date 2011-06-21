/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.sysfera.vishnu.api.vishnu.internal;

public class RestartOp extends EObject {
  private long swigCPtr;

  protected RestartOp(long cPtr, boolean cMemoryOwn) {
    super(VISHNUJNI.SWIGRestartOpUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(RestartOp obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        VISHNUJNI.delete_RestartOp(swigCPtr);
      }
      swigCPtr = 0;
    }
    super.delete();
  }

  public RestartOp() {
    this(VISHNUJNI.new_RestartOp(), true);
  }

  public void _initialize() {
    VISHNUJNI.RestartOp__initialize(swigCPtr, this);
  }

  public String getVishnuConf() {
    return VISHNUJNI.RestartOp_getVishnuConf(swigCPtr, this);
  }

  public void setVishnuConf(String _vishnuConf) {
    VISHNUJNI.RestartOp_setVishnuConf(swigCPtr, this, _vishnuConf);
  }

  public int getSedType() {
    return VISHNUJNI.RestartOp_getSedType(swigCPtr, this);
  }

  public void setSedType(int _sedType) {
    VISHNUJNI.RestartOp_setSedType(swigCPtr, this, _sedType);
  }

  public SWIGTYPE_p_ecorecpp__mapping__any eGet(int _featureID, boolean _resolve) {
    return new SWIGTYPE_p_ecorecpp__mapping__any(VISHNUJNI.RestartOp_eGet(swigCPtr, this, _featureID, _resolve), true);
  }

  public void eSet(int _featureID, SWIGTYPE_p_ecorecpp__mapping__any _newValue) {
    VISHNUJNI.RestartOp_eSet(swigCPtr, this, _featureID, SWIGTYPE_p_ecorecpp__mapping__any.getCPtr(_newValue));
  }

  public boolean eIsSet(int _featureID) {
    return VISHNUJNI.RestartOp_eIsSet(swigCPtr, this, _featureID);
  }

  public void eUnset(int _featureID) {
    VISHNUJNI.RestartOp_eUnset(swigCPtr, this, _featureID);
  }

  public SWIGTYPE_p_ecore__EClass _eClass() {
    long cPtr = VISHNUJNI.RestartOp__eClass(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_ecore__EClass(cPtr, false);
  }

}
