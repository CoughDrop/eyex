#include <node.h>
#include "include\eyex\EyeX.h"

#pragma comment (lib, "Tobii.EyeX.Client.lib")

namespace eyex {
	using v8::Local;
	using v8::Context;
	using v8::Persistent;
	using v8::Isolate;
	using v8::FunctionCallbackInfo;
	using v8::Object;
	using v8::HandleScope;
	using v8::NewStringType;
	using v8::String;
	using v8::Number;
	using v8::Value;
	using v8::Null;
	using v8::Function;
	using node::AtExit;

    static int status = 0;

	void jsHello(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world", NewStringType::kNormal).ToLocalChecked());
	}

	static Isolate* g_callback_isolate = 0;
	static Persistent<Function> g_callback;

	void triggerJsCallback() {
		if (g_callback_isolate != 0) {
			const unsigned argc = 1;
			g_callback_isolate = g_callback_isolate->GetCurrent();
			Local<Context> context = g_callback_isolate->GetCurrentContext();
			Local<Value> argv[argc] = { String::NewFromUtf8(g_callback_isolate, "data", NewStringType::kNormal).ToLocalChecked() };
			Local<Function> callback = Local<Function>::New(g_callback_isolate, g_callback);
			callback->Call(context, Null(g_callback_isolate), argc, argv);
		}
	}

	void jsListen(const FunctionCallbackInfo<Value>& args) {
		printf("listening\n");
		g_callback_isolate = args.GetIsolate();
		g_callback.Reset(g_callback_isolate, Local<Function>::Cast(args[0]));
		args.GetReturnValue().Set(true);
		triggerJsCallback();
	}


	// ID of the global interactor that provides our data stream; must be unique within the application.
	static const TX_STRING InteractorId = "CoughDrop EyeX Module";

	// global variables
	static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

	/*
	* Initializes g_hGlobalInteractorSnapshot with an interactor that has the Fixation Data behavior.
	*/
	bool InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
	{
		TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
		TX_FIXATIONDATAPARAMS params = { TX_FIXATIONDATAMODE_SENSITIVE };
		// options are TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED or TX_GAZEPOINTDATAMODE_UNFILTERED
		TX_GAZEPOINTDATAPARAMS params2 = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
		bool success;

		success = txCreateGlobalInteractorSnapshot(
			hContext,
			InteractorId,
			&g_hGlobalInteractorSnapshot,
			&hInteractor) == TX_RESULT_OK;
		success &= txCreateFixationDataBehavior(hInteractor, &params) == TX_RESULT_OK;
		success &= txCreateGazePointDataBehavior(hInteractor, &params2) == TX_RESULT_OK;

		txReleaseObject(&hInteractor);

		return success;
	}

	/*
	* Callback function invoked when a snapshot has been committed.
	*/
	void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
	{
		// check the result code using an assertion.
		// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

		TX_RESULT result = TX_RESULT_UNKNOWN;
		txGetAsyncDataResultCode(hAsyncData, &result);
		assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
	}

	/*
	* Callback function invoked when the status of the connection to the EyeX Engine has changed.
	*/
	void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
	{
		switch (connectionState) {
		case TX_CONNECTIONSTATE_CONNECTED: {
			bool success;
			status = 2;
			printf("The connection state is now CONNECTED (We are connected to the EyeX Engine)\n");
			// commit the snapshot with the global interactor as soon as the connection to the engine is established.
			// (it cannot be done earlier because committing means "send to the engine".)
			success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
			if (!success) {
			  status = -1;
				printf("Failed to initialize the data stream.\n");
			}
			else
			{
			  status = 3;
				printf("Waiting for fixation data to start streaming...\n");
			}
		}
										   break;

		case TX_CONNECTIONSTATE_DISCONNECTED:
		  status = 5;
			printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
			break;

		case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		  status = 1;
			printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
			break;

		case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		  status = -2;
			printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
			break;

		case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		  status = -3;
			printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
			break;
		}
	}

	static double last_data_x;
	static double last_data_y;
	static double last_data_ts;
	static double last_end_x;
	static double last_end_y;
	static double last_end_ts;
	static double last_begin_x;
	static double last_begin_y;
	static double last_begin_ts;
	static double last_gaze_x;
	static double last_gaze_y;
	static double last_gaze_ts;

	void jsPing(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();
		Local<Object> obj = Object::New(isolate);
		Local<String> str = String::NewFromUtf8(isolate, "x", NewStringType::kNormal).ToLocalChecked();
		obj->Set(context, String::NewFromUtf8(isolate, "status", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, status));
		obj->Set(context, String::NewFromUtf8(isolate, "end_x", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_end_x));
		obj->Set(context, String::NewFromUtf8(isolate, "end_y", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_end_y));
		obj->Set(context, String::NewFromUtf8(isolate, "end_ts", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_end_ts));
		obj->Set(context, String::NewFromUtf8(isolate, "begin_x", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_begin_x));
		obj->Set(context, String::NewFromUtf8(isolate, "begin_y", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_begin_y));
		obj->Set(context, String::NewFromUtf8(isolate, "begin_ts", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_begin_ts));
		obj->Set(context, String::NewFromUtf8(isolate, "data_x", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_data_x));
		obj->Set(context, String::NewFromUtf8(isolate, "data_y", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_data_y));
		obj->Set(context, String::NewFromUtf8(isolate, "data_ts", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_data_ts));
		obj->Set(context, String::NewFromUtf8(isolate, "gaze_x", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_gaze_x));
		obj->Set(context, String::NewFromUtf8(isolate, "gaze_y", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_gaze_y));
		obj->Set(context, String::NewFromUtf8(isolate, "gaze_ts", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, last_gaze_ts));

		args.GetReturnValue().Set(obj);
	}
	/*
	* Handles an event from the fixation data stream.
	*/
	void OnFixationDataEvent(TX_HANDLE hFixationDataBehavior)
	{
		TX_FIXATIONDATAEVENTPARAMS eventParams;
		TX_FIXATIONDATAEVENTTYPE eventType;
		char* eventDescription;

		if (txGetFixationDataEventParams(hFixationDataBehavior, &eventParams) == TX_RESULT_OK) {
		  status = 4;
			eventType = eventParams.EventType;

			double x = eventParams.X;
			double y = eventParams.Y;
			double ts = eventParams.Timestamp;
			if (eventType == TX_FIXATIONDATAEVENTTYPE_DATA) {
				last_data_x = x;
				last_data_y = y;
				last_data_ts = ts;
				eventDescription = "Data";
			}
			else if (eventType == TX_FIXATIONDATAEVENTTYPE_END) {
				last_end_x = x;
				last_end_y = y;
				last_end_ts = ts;
				eventDescription = "End";
			}
			else {
				last_begin_x = x;
				last_begin_y = y;
				last_begin_ts = ts;
				eventDescription = "Begin";
			}
			eventDescription = (eventType == TX_FIXATIONDATAEVENTTYPE_DATA) ? "Data"
				: ((eventType == TX_FIXATIONDATAEVENTTYPE_END) ? "End"
					: "Begin");
//			printf("Fixation %s: (%.1f, %.1f) timestamp %.0f ms\n", eventDescription, eventParams.X, eventParams.Y, eventParams.Timestamp);
		}
		else {
			printf("Failed to interpret fixation data event packet.\n");
		}
	}

	/*
	* Handles an event from the Gaze Point data stream.
	*/
	void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
	{
		TX_GAZEPOINTDATAEVENTPARAMS eventParams;
		if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		  status = 4;
			last_gaze_x = eventParams.X;
			last_gaze_y = eventParams.Y;
			last_gaze_ts = eventParams.Timestamp;
//			printf("Gaze Data: (%.1f, %.1f) timestamp %.0f ms\n", eventParams.X, eventParams.Y, eventParams.Timestamp);
		}
		else {
			printf("Failed to interpret gaze data event packet.\n");
		}
	}

	/*
	* Callback function invoked when an event has been received from the EyeX Engine.
	*/
	void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
	{
		TX_HANDLE hEvent = TX_EMPTY_HANDLE;
		TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

		txGetAsyncDataContent(hAsyncData, &hEvent);

		// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
		//OutputDebugStringA(txDebugObject(hEvent));

		if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_FIXATIONDATA) == TX_RESULT_OK) {
			OnFixationDataEvent(hBehavior);
			txReleaseObject(&hBehavior);
		} else if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
			OnGazeDataEvent(hBehavior);
			txReleaseObject(&hBehavior);
		}

		// NOTE since this is a very simple application with a single interactor and a single data stream, 
		// our event handling code can be very simple too. A more complex application would typically have to 
		// check for multiple behaviors and route events based on interactor IDs.

		txReleaseObject(&hEvent);
	}

	static TX_CONTEXTHANDLE g_hContext = TX_EMPTY_HANDLE;
	bool setup() {
		TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
		TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
		TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
		bool success;

		// initialize and enable the context that is our link to the EyeX Engine.
		success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
		success &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
		success &= InitializeGlobalInteractorSnapshot(hContext);
		success &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
		success &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
		success &= txEnableConnection(hContext) == TX_RESULT_OK;

		g_hContext = hContext;

		// let the events flow until a key is pressed.
		if (success) {
		  status = 10;
			printf("Initialization was successful.\n");
		}
		else {
		  status = -10;
			printf("Initialization failed.\n");
		}
		
		return success;
	}

	bool teardown() {
		TX_CONTEXTHANDLE hContext = g_hContext;
		bool success;

		// disable and delete the context.
		txDisableConnection(hContext);
		txReleaseObject(&g_hGlobalInteractorSnapshot);
		success = txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
		success &= txReleaseContext(&hContext) == TX_RESULT_OK;
		success &= txUninitializeEyeX() == TX_RESULT_OK;
		if (!success) {
			printf("EyeX could not be shut down cleanly. Did you remember to release all handles?\n");
		}
		status = 0;
		return success;
	}

	void jsSetup(const FunctionCallbackInfo<Value>& args) {
		bool result = setup();
		Isolate* isolate = args.GetIsolate();
		args.GetReturnValue().Set(result);
	}

	void jsTeardown(const FunctionCallbackInfo<Value>& args) {
		bool result = teardown();
		Isolate* isolate = args.GetIsolate();
		args.GetReturnValue().Set(result);
	}

	void init(Local<Object> exports) {
		NODE_SET_METHOD(exports, "hello", jsHello);
		NODE_SET_METHOD(exports, "setup", jsSetup);
		NODE_SET_METHOD(exports, "teardown", jsTeardown);
		NODE_SET_METHOD(exports, "listen", jsListen);
		NODE_SET_METHOD(exports, "ping", jsPing);
	}

	NODE_MODULE(eyex, init)
}