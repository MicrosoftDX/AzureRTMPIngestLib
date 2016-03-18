# Azure RTMP Ingest Library
Last updated 3/18/2016

## Prerequsites:
1. Microsoft Visual Studio 2015 Update 1
2. Install Microsoft Universal Smooth Streaming Client SDK from Visual Studio... Tools...Extensions and Updates...
3. Install [Microsoft Media Platform Player Framework](http://playerframework.codeplex.com)
4. Install Visual Studio Extensibility Tools if you want to build the SDK

## Azure Setup:
1. Create an Azure account on WindowsAzure.com
2. Create a Media Services instance on the account
3. Once created get the name of the instance and the primary key
4. Create a new live channel with 
   1. "none" encoding, 
   2. RTMP ingest protocol
   3. No IP restrictions
   4. Keep "Start the channel now" and "Add one streaming unit" checked
5. Create a program in the channel