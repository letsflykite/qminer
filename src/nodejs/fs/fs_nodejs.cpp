#include "fs_nodejs.h"

///////////////////////////////
// NodeJs-Directory
TNodeJsFPath::TNodeJsFPath(const TStr& FPath): CanonicalFPath(GetCanonicalPath(FPath)) { }

bool TNodeJsFPath::Equals(const TNodeJsFPath& JsFPath) const {
    return CanonicalFPath == JsFPath.GetFPath();
}

bool TNodeJsFPath::IsSubdir(const TNodeJsFPath& JsFPath) const {
    return CanonicalFPath.StartsWith(JsFPath.GetFPath());
}

void TNodeJsFPath::GetFPathV(const TStrV& FPathV, TVec<TNodeJsFPath>& JsFPathV) {
    for (TStrV::TIter It = FPathV.BegI(); It != FPathV.EndI(); ++It) {
        JsFPathV.Add(TNodeJsFPath(*It));
    }
}

TStr TNodeJsFPath::GetCanonicalPath(const TStr& FPath) {
    // Get absolute path
    TStr AbsFPath = TStr::GetNrAbsFPath(FPath);
    // Remove any redundancies
    TStrV CanonV; AbsFPath.SplitOnAllCh('/', CanonV);
    TSStack<TStr> CanonS; TStr CurrStr;
    for (int ElN = 0; ElN < CanonV.Len(); ++ElN) {
        CurrStr = CanonV.GetVal(ElN);
        if (CurrStr == "..") {
            EAssertR(!CanonS.Empty(), "Stack empty");
            CanonS.Pop();
        } else if (CurrStr != ".") {
            CanonS.Push(CurrStr+"/");
        }
    }
    // Assemble the canonical path (from left to right
    EAssertR(!CanonS.Empty(), "Stack empty");
    // We start with drive letter (Windows) or slash (Unix)
    TChA CanonFPath = AbsFPath.LeftOf('/'); CanonFPath += '/';
    // Get the rest of the path
    for (int CanonN = CanonS.Len() - 1; CanonN >= 0; CanonN--) {
        CanonFPath += CanonS[CanonN];
    }
    // Done
    return CanonFPath;
}

///////////////////////////////
// NodeJs-Filesystem
void TNodeJsFs::Init(v8::Handle<v8::Object> exports) {
    // Add all prototype methods, getters and setters here.
    NODE_SET_METHOD(exports, "openRead", _openRead);
    NODE_SET_METHOD(exports, "openWrite", _openWrite);
    NODE_SET_METHOD(exports, "openAppend", _openAppend);
    NODE_SET_METHOD(exports, "exists", _exists);
    NODE_SET_METHOD(exports, "copy", _copy);
    NODE_SET_METHOD(exports, "move", _move);
    NODE_SET_METHOD(exports, "del", _del);
    NODE_SET_METHOD(exports, "rename", _rename);
    NODE_SET_METHOD(exports, "fileInfo", _fileInfo);
    NODE_SET_METHOD(exports, "mkdir", _mkdir);
    NODE_SET_METHOD(exports, "rmdir", _rmdir);
    NODE_SET_METHOD(exports, "listFile", _listFile);
}

void TNodeJsFs::openRead(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(), "Expected file path.");
    TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
    // file exist check is done by TFIn
    Args.GetReturnValue().Set(TNodeJsFIn::New(FNm));
}

void TNodeJsFs::openWrite(const v8::FunctionCallbackInfo<v8::Value>& Args) { // Call withb AppendP = false
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(), "Expected file path.");
    TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
    Args.GetReturnValue().Set(TNodeJsFOut::New(FNm, false));
}

void TNodeJsFs::openAppend(const v8::FunctionCallbackInfo<v8::Value>& Args) { // Call with AppendP = true 
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(), "Expected file path.");
    TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
    Args.GetReturnValue().Set(TNodeJsFOut::New(FNm, true));
}

void TNodeJsFs::exists(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(), "Expected file path.");
    TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
    Args.GetReturnValue().Set(v8::Boolean::New(Isolate, TFile::Exists(FNm)));
}

void TNodeJsFs::copy(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 2 && Args[0]->IsString() && Args[1]->IsString(),
        "Expected 2 arguments: source and destination file paths.");
    TStr SrcFNm(*v8::String::Utf8Value(Args[0]->ToString()));
    EAssertR(TFile::Exists(SrcFNm), "File '" + SrcFNm + "' does not exist");
    TStr DstFNm(*v8::String::Utf8Value(Args[1]->ToString()));
    TFile::Copy(SrcFNm, DstFNm);
    Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsFs::move(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 2 && Args[0]->IsString() && Args[1]->IsString(),
        "Expected 2 arguments: source and destination file paths.");
    TStr SrcFNm(*v8::String::Utf8Value(Args[0]->ToString()));
    EAssertR(TFile::Exists(SrcFNm), TStr("File '" + SrcFNm + "' does not exist").CStr());
    TStr DstFNm(*v8::String::Utf8Value(Args[1]->ToString()));
    TFile::Copy(SrcFNm, DstFNm);
    TFile::Del(SrcFNm, false); // ThrowExceptP = false 
    Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsFs::del(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(),
        "Expected a file path as the only argument.");
    TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
    EAssertR(TFile::Exists(FNm), TStr("File '" + FNm + "' does not exist").CStr());
    TFile::Del(FNm, false); // ThrowExceptP = false 
    Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsFs::rename(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 2 && Args[0]->IsString() && Args[1]->IsString(),
        "Expected 2 arguments: source and destination file paths.");
    TStr SrcFNm(*v8::String::Utf8Value(Args[0]->ToString()));
    EAssertR(TFile::Exists(SrcFNm), TStr("File '" + SrcFNm + "' does not exist").CStr());
    TStr DstFNm(*v8::String::Utf8Value(Args[1]->ToString()));
    TFile::Rename(SrcFNm, DstFNm);
    Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsFs::fileInfo(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(),
        "Expected a file path as the only argument.");
    TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
    EAssertR(TFile::Exists(FNm), TStr("File '" + FNm + "' does not exist").CStr());
    const uint64 CreateTm = TFile::GetCreateTm(FNm);
    const uint64 LastAccessTm = TFile::GetLastAccessTm(FNm);
    const uint64 LastWriteTm = TFile::GetLastWriteTm(FNm);
    const uint64 Size = TFile::GetSize(FNm);
    v8::Handle<v8::Object> Obj = v8::Object::New(Isolate);
    Obj->Set(v8::String::NewFromUtf8(Isolate, "createTime"),
        v8::String::NewFromUtf8(Isolate, TTm::GetTmFromMSecs(CreateTm).GetWebLogDateTimeStr().CStr()));
    Obj->Set(v8::String::NewFromUtf8(Isolate, "lastAccessTime"),
        v8::String::NewFromUtf8(Isolate, TTm::GetTmFromMSecs(LastAccessTm).GetWebLogDateTimeStr().CStr()));
    Obj->Set(v8::String::NewFromUtf8(Isolate, "lastWriteTime"),
        v8::String::NewFromUtf8(Isolate, TTm::GetTmFromMSecs(LastWriteTm).GetWebLogDateTimeStr().CStr()));
    Obj->Set(v8::String::NewFromUtf8(Isolate, "size"),
        v8::Number::New(Isolate, static_cast<double>(Size)));
    Args.GetReturnValue().Set(Obj);
}

void TNodeJsFs::mkdir(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(),
        "Expected directory name as the only argument.");
    TStr FPath(*v8::String::Utf8Value(Args[0]->ToString()));
    const bool GenDirP = TDir::GenDir(FPath);
    Args.GetReturnValue().Set(v8::Boolean::New(Isolate, GenDirP));
}

void TNodeJsFs::rmdir(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() == 1 && Args[0]->IsString(),
        "Expected directory name as the only argument.");
    TStr FPath(*v8::String::Utf8Value(Args[0]->ToString()));
    const bool DelDirP = TDir::DelDir(FPath);
    Args.GetReturnValue().Set(v8::Boolean::New(Isolate, DelDirP));
}

void TNodeJsFs::listFile(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    EAssertR(Args.Length() >= 1 && Args[0]->IsString(),
        "Expected directory path as the first argument.");
    // Read parameters
    TStr FPath(*v8::String::Utf8Value(Args[0]->ToString()));
    TStrV FExtV;    
    if (Args.Length() >= 2 && Args[1]->IsString()) {
        FExtV.Add(TStr(*v8::String::Utf8Value(Args[1]->ToString())));
    }
    const bool RecurseP = Args.Length() >= 3 && Args[2]->IsBoolean() && Args[2]->BooleanValue();
    // Get file list
    TStrV FNmV;
    TFFile::GetFNmV(FPath, FExtV, RecurseP, FNmV);
    FNmV.Sort();
    v8::Handle<v8::Array> FNmArr = v8::Array::New(Isolate, FNmV.Len());
    for(int FldN = 0; FldN < FNmV.Len(); ++FldN) {
        FNmArr->Set(v8::Integer::New(Isolate, FldN), v8::String::NewFromUtf8(Isolate, FNmV.GetVal(FldN).CStr()));
    }
    Args.GetReturnValue().Set(FNmArr);
}

///////////////////////////////
// NodeJs-FIn
v8::Persistent<v8::Function> TNodeJsFIn::constructor;
TStr TNodeJsFIn::ClassId = "FIn";

void TNodeJsFIn::Init(v8::Handle<v8::Object> exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, _New);
	
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, ClassId.CStr()));
	// ObjectWrap uses the first internal field to store the wrapped pointer
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all prototype methods, getters and setters here
	NODE_SET_PROTOTYPE_METHOD(tpl, "peekCh", _peekCh);
	NODE_SET_PROTOTYPE_METHOD(tpl, "getCh", _getCh);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readLine", _readLine);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readAll", _readAll);

	tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "eof"), _eof);
	tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "length"), _length);
	
	// This has to be last, otherwise the properties won't show up on the
	// object in JavaScript	
	constructor.Reset(Isolate, tpl->GetFunction());
#ifndef MODULE_INCLUDE_FS
	exports->Set(v8::String::NewFromUtf8(Isolate, "FIn"),
		tpl->GetFunction());
#endif

}

v8::Local<v8::Object> TNodeJsFIn::New(const TStr& FNm) {
	// called from C++	
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope HandleScope(Isolate);
	// create an instance using the constructor
	EAssertR(!constructor.IsEmpty(), "TNodeJsFIn::New: constructor is empty. Did you call TNodeJsFIn::Init(exports); in this module's init function?");
    v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(Isolate, constructor);
    // no arguments to constructor
	v8::Local<v8::Object> Instance = cons->NewInstance();
	// wrap our C++ object
	return HandleScope.Escape(
		TNodeJsUtil::WrapJsInstance(Instance, new TNodeJsFIn(FNm)));
}

void TNodeJsFIn::New(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	EAssertR(Args.IsConstructCall(), "TNodeJsFIn: not a constructor call (you forgot to use the new operator)");
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);
	// set hidden class id
	v8::Local<v8::Object> Instance = Args.This();
	v8::Handle<v8::String> key = v8::String::NewFromUtf8(Isolate, "class");
	v8::Handle<v8::String> value = v8::String::NewFromUtf8(Isolate, ClassId.CStr());
	Instance->SetHiddenValue(key, value);
	// empty constructor call just forwards the instance
	if (Args.Length() == 0) { Args.GetReturnValue().Set(Instance); return; }
	// parse arguments
	EAssertR(Args.Length() == 1 && Args[0]->IsString(),
		"Expected a file path.");
	TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
	//EAssertR(TFile::Exists(FNm), "File does not exist.");
    // Args.This() is an instance, wrap our C++ object
	Args.GetReturnValue().Set(
		TNodeJsUtil::WrapJsInstance(Instance, new TNodeJsFIn(FNm)));
}

void TNodeJsFIn::peekCh(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFIn* JsFIn = ObjectWrap::Unwrap<TNodeJsFIn>(Args.This());

    Args.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, TStr(JsFIn->SIn->PeekCh()).CStr()));
}

void TNodeJsFIn::getCh(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFIn* JsFIn = ObjectWrap::Unwrap<TNodeJsFIn>(Args.This());

    Args.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, TStr(JsFIn->SIn->GetCh()).CStr()));
}

void TNodeJsFIn::readLine(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFIn* JsFIn = ObjectWrap::Unwrap<TNodeJsFIn>(Args.This());
    TChA LnChA; JsFIn->SIn->GetNextLnBf(LnChA);

    Args.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, LnChA.CStr()));
}

void TNodeJsFIn::readAll(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFIn* JsFIn = ObjectWrap::Unwrap<TNodeJsFIn>(Args.This());
    TStr Res = TStr::LoadTxt(JsFIn->SIn);

    Args.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, Res.CStr()));
}

void TNodeJsFIn::eof(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFIn* JsFIn = ObjectWrap::Unwrap<TNodeJsFIn>(Info.Holder());

    Info.GetReturnValue().Set(v8::Boolean::New(Isolate, JsFIn->SIn->Eof()));
}

void TNodeJsFIn::length(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFIn* JsFIn = ObjectWrap::Unwrap<TNodeJsFIn>(Info.Holder());

    Info.GetReturnValue().Set(v8::Integer::New(Isolate, JsFIn->SIn->Len()));
}

///////////////////////////////
// NodeJs-FOut
v8::Persistent<v8::Function> TNodeJsFOut::constructor;
TStr TNodeJsFOut::ClassId = "FOut";

void TNodeJsFOut::Init(v8::Handle<v8::Object> exports) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, _New);
    tpl->SetClassName(v8::String::NewFromUtf8(Isolate, "FOut"));
    // ObjectWrap uses the first internal field to store the wrapped pointer
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Add all prototype methods, getters and setters here.
    NODE_SET_PROTOTYPE_METHOD(tpl, "write", _write);
    NODE_SET_PROTOTYPE_METHOD(tpl, "writeLine", _writeLine);
    NODE_SET_PROTOTYPE_METHOD(tpl, "flush", _flush);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", _close);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript
    constructor.Reset(Isolate, tpl->GetFunction());
    #ifndef MODULE_INCLUDE_FS
    exports->Set(v8::String::NewFromUtf8(Isolate, "FOut"),
        tpl->GetFunction());
    #endif
}

v8::Local<v8::Object> TNodeJsFOut::New(const TStr& FNm, const bool& AppendP) {
    // called from C++	
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope HandleScope(Isolate);
	// create an instance using the constructor
	EAssertR(!constructor.IsEmpty(), "TNodeJsFOut::New: constructor is empty. Did you call TNodeJsFOut::Init(exports); in this module's init function?");
	v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(Isolate, constructor);		
	// no arguments to constructor
	v8::Local<v8::Object> Instance = cons->NewInstance();
	// wrap our C++ object
	return HandleScope.Escape(
		TNodeJsUtil::WrapJsInstance(Instance, new TNodeJsFOut(FNm, AppendP)));
} 

void TNodeJsFOut::New(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	EAssertR(Args.IsConstructCall(), "TNodeJsFOut: not a constructor call (you forgot to use the new operator)");
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope HandleScope(Isolate);
	// set hidden class id
	v8::Local<v8::Object> Instance = Args.This();
	v8::Handle<v8::String> key = v8::String::NewFromUtf8(Isolate, "class");
	v8::Handle<v8::String> value = v8::String::NewFromUtf8(Isolate, ClassId.CStr());
	Instance->SetHiddenValue(key, value);		
	// empty constructor call just forwards the instance
	if (Args.Length() == 0) { Args.GetReturnValue().Set(Instance); return; }
	// parse arguments
	EAssertR(Args.Length() >= 1 && Args[0]->IsString(),
		"Expected file path.");
	TStr FNm(*v8::String::Utf8Value(Args[0]->ToString()));
	bool AppendP = Args.Length() >= 2 && Args[1]->IsBoolean() && Args[1]->BooleanValue();
	// Args.This() is an instance, wrap our C++ object
	Args.GetReturnValue().Set(
		TNodeJsUtil::WrapJsInstance(Instance, new TNodeJsFOut(FNm, AppendP)));
}

void TNodeJsFOut::write(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);
    EAssertR(Args.Length() == 1, "Invalid number of arguments to fout.write()");
    TNodeJsFOut* JsFOut = ObjectWrap::Unwrap<TNodeJsFOut>(Args.This());
    EAssertR(!JsFOut->SOut.Empty(), "Output stream already closed!");
    if (Args[0]->IsString()) {
        JsFOut->SOut->PutStr(*v8::String::Utf8Value(Args[0]->ToString()));
    } else if (Args[0]->IsInt32()) {
        JsFOut->SOut->PutStr(TInt::GetStr(Args[0]->Int32Value()));
	} else if (Args[0]->IsNumber()) {
		JsFOut->SOut->PutStr(TFlt::GetStr(Args[0]->NumberValue()));
	} else if (TNodeJsUtil::IsArgJson(Args, 0)) {
		JsFOut->SOut->PutStr(TJsonVal::GetStrFromVal(TNodeJsUtil::GetArgJson(Args, 0)));
	} else {
        EFailR("Invalid type passed to fout.write() function.");
    }
    
    Args.GetReturnValue().Set(Args.Holder());
}

void TNodeJsFOut::writeLine(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    write(Args);

    TNodeJsFOut* JsFOut = ObjectWrap::Unwrap<TNodeJsFOut>(Args.This());
    EAssertR(!JsFOut->SOut.Empty(), "Output stream already closed!");
    JsFOut->SOut->PutLn();

    Args.GetReturnValue().Set(Args.Holder());
}

void TNodeJsFOut::flush(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFOut* JsFOut = ObjectWrap::Unwrap<TNodeJsFOut>(Args.This());
    JsFOut->SOut->Flush();

    Args.GetReturnValue().Set(Args.Holder());
}

void TNodeJsFOut::close(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsFOut* JsFOut = ObjectWrap::Unwrap<TNodeJsFOut>(Args.This());
    JsFOut->SOut.Clr();

    Args.GetReturnValue().Set(Args.Holder());
}

#ifndef MODULE_INCLUDE_FS
///////////////////////////////
// Register functions, etc.  
void init(v8::Handle<v8::Object> exports) {//, v8::Handle<v8::Object> module
	/*v8::Local<v8::String> filename =
		module->Get(v8::String::NewSymbol("filename")).As<v8::String>();
	TNodeJsUtil::*/

    TNodeJsFs::Init(exports);
    TNodeJsFIn::Init(exports);
    TNodeJsFOut::Init(exports);
}

NODE_MODULE(fs, init)
#endif

