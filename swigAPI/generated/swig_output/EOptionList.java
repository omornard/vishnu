/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.sysfera.vishnu.api.vishnu.internal;

public class EOptionList {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected EOptionList(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(EOptionList obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        VISHNUJNI.delete_EOptionList(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void insert_all(EOptionList _q) {
    VISHNUJNI.EOptionList_insert_all(swigCPtr, this, EOptionList.getCPtr(_q), _q);
  }

  public void insert_at(long _pos, OptionValue _obj) {
    VISHNUJNI.EOptionList_insert_at(swigCPtr, this, _pos, OptionValue.getCPtr(_obj), _obj);
  }

  public OptionValue get(long _index) {
    long cPtr = VISHNUJNI.EOptionList_get(swigCPtr, this, _index);
    return (cPtr == 0) ? null : new OptionValue(cPtr, false);
  }

  public void push_back(OptionValue _obj) {
    VISHNUJNI.EOptionList_push_back(swigCPtr, this, OptionValue.getCPtr(_obj), _obj);
  }

  public long size() {
    return VISHNUJNI.EOptionList_size(swigCPtr, this);
  }

  public void clear() {
    VISHNUJNI.EOptionList_clear(swigCPtr, this);
  }

}
