//
//  CCMrubyEngine.cpp
//  CocosMruby
//
//  Created by admin on 13/09/17.
//
//

#include "CCMrubyEngine.h"
#include "mruby.h"
#include "mruby/array.h"
#include "mruby/class.h"
#include "mruby/compile.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include "MrubyCocos2d.h"
#include "MrubyCocosDenshion.h"
#include "MrubyBox2D.h"

extern "C" mrb_value mrb_get_backtrace(mrb_state*, mrb_value);

static const char* getBaseName(const char* fullpath) {
  int len = strlen(fullpath);
  for (int i = len - 1; --i >= 0; ) {
    if (fullpath[i] == '/')
      return &fullpath[i + 1];
  }
  return fullpath;
}

static struct RClass* getMrubyCocos2dClassPtr(mrb_state *mrb, const char* className) {
  RClass* mod = mrb_class_get_under(mrb, mrb->kernel_module, "Cocos2d");
  return mrb_class_get_under(mrb, mod, className);
}

static mrb_value getMrubyCocos2dClassValue(mrb_state *mrb, const char* className) {
  return mrb_obj_value(getMrubyCocos2dClassPtr(mrb, className));
}

int registerProc(mrb_state *mrb, mrb_value proc) {
  mrb_value man = getMrubyCocos2dClassValue(mrb, "HandleManager");
  mrb_value result = mrb_funcall(mrb, man, "register", 1, proc);
  return mrb_fixnum(result);
}

mrb_value getRegisteredProc(mrb_state *mrb, int id) {
  mrb_value man = getMrubyCocos2dClassValue(mrb, "HandleManager");
  mrb_value proc = mrb_funcall(mrb, man, "getProc", 1, mrb_fixnum_value(id));
  return proc;
}

NS_CC_BEGIN

static const char HelperFunctions[] =
"module Cocos2d\n"
"  class HandleManager\n"
"    @@counter = 0\n"
"    @@proc2id = {}\n"
"    @@id2proc = {}\n"

"    def self.register(proc)\n"
"      if @@proc2id.has_key?(proc)\n"
"        return @@proc2id[proc]\n"
"      end\n"
"      id = (@@counter += 1)\n"
"      @@id2proc[id] = proc\n"
"      @@proc2id[proc] = id\n"
"      return id\n"
"    end\n"

"    def self.getProc(id)\n"
"      return @@id2proc[id]\n"
"    end\n"
"  end\n"
"end\n";

////////////////////////////////////////////////////////////////
// Misc.
static mrb_value _mrb_puts(mrb_state *mrb, mrb_value self) {
  mrb_value* args;
  int n;
  mrb_get_args(mrb, "*", &args, &n);
  for (int i = 0; i < n; ++i) {
    mrb_value s = mrb_funcall(mrb, args[i], "to_s", 0);
    CCLOG("%s", RSTRING_PTR(s));
  }
  return mrb_nil_value();
}

static int dumpException(mrb_state *mrb) {
  if (!mrb->exc)
    return FALSE;
  
  mrb_value exc = mrb_obj_value(mrb->exc);
  mrb_value backtrace = mrb_get_backtrace(mrb, exc);
  mrb_value s = mrb_funcall(mrb, exc, "inspect", 0);
  CCLOG("%s", RSTRING_PTR(s));
  for (mrb_int n = mrb_ary_len(mrb, backtrace), i = 0; i < n; ++i) {
    mrb_value v = mrb_ary_ref(mrb, backtrace, i);
    CCLOG("  %s", RSTRING_PTR(v));
  }
  mrb->exc = 0;
  return TRUE;
}


CCMrubyEngine* CCMrubyEngine::m_defaultEngine = NULL;

CCMrubyEngine* CCMrubyEngine::defaultEngine(void)
{
  if (!m_defaultEngine)
  {
    m_defaultEngine = new CCMrubyEngine();
    m_defaultEngine->init();
  }
  return m_defaultEngine;
}

CCMrubyEngine::~CCMrubyEngine()
{
  mrb_close(m_mrb);
  m_defaultEngine = NULL;
}

CCMrubyEngine::CCMrubyEngine(void)
{
}

bool CCMrubyEngine::init(void)
{
  m_mrb = mrb_open();
  
  // Installs general functions.
  mrb_define_method(m_mrb, m_mrb->kernel_module, "cclog", _mrb_puts, MRB_ARGS_ANY());
  mrb_define_method(m_mrb, m_mrb->kernel_module, "puts", _mrb_puts, MRB_ARGS_ANY());
  mrb_define_method(m_mrb, m_mrb->kernel_module, "p", _mrb_puts, MRB_ARGS_ANY());

  // Installs cocos2d classes.
  installMrubyCocos2d(m_mrb);
  installMrubyCocosDenshion(m_mrb);
  installMrubyBox2D(m_mrb);
    
  // Installs helper functions.
  // This line must be after installing mruby bindings,
  // because it creates new module for `Cocos2d'.
  mrb_load_string(m_mrb, HelperFunctions);
  
  return true;
}

/** Remove script object. */
void CCMrubyEngine::removeScriptObjectByCCObject(CCObject* pObj)
{
  CCLOGERROR("CCMrubyEngine::removeScriptObjectByCCObject has not implemented: %p", pObj);
}

int CCMrubyEngine::executeString(const char* codes)
{
  int arena = mrb_gc_arena_save(m_mrb);
#if DEBUG
  mrbc_context *cxt = mrbc_context_new(m_mrb);
  mrbc_filename(m_mrb, cxt, getBaseName("*main*"));
  mrb_load_string_cxt(m_mrb, codes, cxt);
  mrbc_context_free(m_mrb, cxt);
#else
  // No debug info.
  mrb_load_string(m_mrb, codes);
#endif
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeScriptFile(const char* filename)
{
  unsigned long nSize = 0;
  unsigned char* pBuffer = CCFileUtils::sharedFileUtils()->getFileData(filename, "r", &nSize);
  if (pBuffer == NULL)
    return FALSE;

  int arena = mrb_gc_arena_save(m_mrb);
#if DEBUG
  mrbc_context *cxt = mrbc_context_new(m_mrb);
  mrbc_filename(m_mrb, cxt, getBaseName(filename));
  mrb_load_nstring_cxt(m_mrb, reinterpret_cast<const char*>(pBuffer), nSize, cxt);
  mrbc_context_free(m_mrb, cxt);
#else
  // No debug info.
  mrb_load_nstring(m_mrb, reinterpret_cast<const char*>(pBuffer), nSize);
#endif
  delete[] pBuffer;
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeGlobalFunction(const char* functionName)
{
  int arena = mrb_gc_arena_save(m_mrb);
  mrb_funcall(m_mrb, mrb_top_self(m_mrb), functionName, 0);
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeNodeEvent(CCNode* pNode, int nAction)
{
  int nHandler = pNode->getScriptHandler();
  if (!nHandler) return 0;
  
  int arena = mrb_gc_arena_save(m_mrb);
  mrb_value proc = getRegisteredProc(m_mrb, nHandler);
  mrb_funcall(m_mrb, proc, "call", 1, mrb_fixnum_value(nAction));
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeMenuItemEvent(CCMenuItem* pMenuItem)
{
  int nHandler = pMenuItem->getScriptTapHandler();
  if (!nHandler) return 0;

  int arena = mrb_gc_arena_save(m_mrb);
  mrb_value block = getRegisteredProc(m_mrb, nHandler);
  mrb_yield(m_mrb, block, mrb_nil_value());
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeNotificationEvent(CCNotificationCenter* pNotificationCenter, const char* pszName)
{
  CCLOGERROR("CCMrubyEngine::executeNotificationEvent has not implemented: %p, %s", pNotificationCenter, pszName);
  return 0;
}

int CCMrubyEngine::executeCallFuncActionEvent(CCCallFunc* pAction, CCObject* pTarget)
{
  CCLOGERROR("CCMrubyEngine::executeCallFuncActionEvent has not implemented: %p, %p", pAction, pTarget);
  return 0;
}

int CCMrubyEngine::executeSchedule(int nHandler, float dt, CCNode* pNode)
{
  if (!nHandler) return FALSE;

  int arena = mrb_gc_arena_save(m_mrb);
  mrb_value block = getRegisteredProc(m_mrb, nHandler);
  mrb_yield(m_mrb, block, mrb_float_value(m_mrb, dt));
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeLayerTouchesEvent(CCLayer* pLayer, int eventType, CCSet *pTouches)
{
  CCLOGERROR("CCMrubyEngine::executeLayerTouchesEvent has not implemented: %p, %d, %p", pLayer, eventType, pTouches);
  return 0;
}

int CCMrubyEngine::executeLayerTouchEvent(CCLayer* pLayer, int eventType, CCTouch *pTouch)
{
  CCTouchScriptHandlerEntry* pScriptHandlerEntry = pLayer->getScriptTouchHandlerEntry();
  if (!pScriptHandlerEntry) return FALSE;
  int nHandler = pScriptHandlerEntry->getHandler();
  if (!nHandler) return FALSE;
  
  int arena = mrb_gc_arena_save(m_mrb);
  const CCPoint pt = CCDirector::sharedDirector()->convertToGL(pTouch->getLocationInView());
  mrb_value proc = getRegisteredProc(m_mrb, nHandler);
  mrb_value args[3];
  args[0] = mrb_fixnum_value(eventType);
  args[1] = mrb_float_value(m_mrb, pt.x);
  args[2] = mrb_float_value(m_mrb, pt.y);
  mrb_yield_argv(m_mrb, proc, 3, args);
  int exc = dumpException(m_mrb);
  mrb_gc_arena_restore(m_mrb, arena);
  return !exc;
}

int CCMrubyEngine::executeLayerKeypadEvent(CCLayer* pLayer, int eventType)
{
  CCLOGERROR("CCMrubyEngine::executeLayerKeypadEvent has not implemented: %p, %d", pLayer, eventType);
  return 0;
}

int CCMrubyEngine::executeAccelerometerEvent(CCLayer* pLayer, CCAcceleration* pAccelerationValue)
{
  CCLOGERROR("CCMrubyEngine::executeAccelerometerEvent has not implemented: %p, %p", pLayer, pAccelerationValue);
  return 0;
}

int CCMrubyEngine::executeEvent(int nHandler, const char* pEventName, CCObject* pEventSource, const char* pEventSourceClassName)
{
  CCLOGERROR("CCMrubyEngine::executeEvent has not implemented: %d, %s, %p, %s", nHandler, pEventName, pEventSource, pEventSourceClassName);
  return 0;
}


/** called by CCAssert to allow scripting engine to handle failed assertions
 * @return true if the assert was handled by the script engine, false otherwise.
 */
bool CCMrubyEngine::handleAssert(const char *msg)
{
  CCLOGERROR("CCMrubyEngine::handleAssert has not implemented: %s", msg);
  return 0;
}

NS_CC_END
