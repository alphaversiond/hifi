//
//  InterfaceActivity.java
//  gvr-interface/java
//
//  Created by Stephen Birarda on 1/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

package io.highfidelity.hifiinterface;

import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.WindowManager;
import android.util.Log;
import org.qtproject.qt5.android.bindings.QtActivity;

import com.google.vr.cardboard.DisplaySynchronizer;
import com.google.vr.cardboard.DisplayUtils;
import com.google.vr.ndk.base.GvrApi;

public class InterfaceActivity extends QtActivity {
    
    public static native void handleHifiURL(String hifiURLString);
    private native long nativeOnCreate(AssetManager assetManager, long gvrContextPtr);

    private AssetManager assetManager;

    // Opaque native pointer to the Application C++ object.
    // This object is owned by the InterfaceActivity instance and passed to the native methods.
    //private long nativeGvrApi;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        // Get the intent that started this activity in case we have a hifi:// URL to parse
        Intent intent = getIntent();
        if (intent.getAction() == Intent.ACTION_VIEW) {
            Uri data = intent.getData();
        
            if (data.getScheme().equals("hifi")) {
                handleHifiURL(data.toString());
            }
        }
        
        DisplaySynchronizer displaySynchronizer = new DisplaySynchronizer(this, DisplayUtils.getDefaultDisplay(this));
        GvrApi gvrApi = new GvrApi(this, displaySynchronizer);
        Log.d("GVR", "gvrApi.toString(): " + gvrApi.toString());

        assetManager = getResources().getAssets();

        //nativeGvrApi =
            nativeOnCreate(assetManager, gvrApi.getNativeGvrContext());

    }
}