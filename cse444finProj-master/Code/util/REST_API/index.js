var express = require('express');
var app = express();

//const fs = require('fs'); 
//const readline = require('readline'); 
const lineReader = require('line-reader'); 
const bodyParser = require("body-parser");
const cors=require("cors");
////const {spawn} = require('child_process');
const { exec } = require('child_process');
var each = require('async-each');

var corsOptions = 
    {
        origin: '*',
        optionsSuccessStatus: 200,
    }

app.use(cors(corsOptions));
app.use(bodyParser.json());

app.get("/callCcode",function(req,res)
{
    console.log("in call c code part");
    
        var options = {
            timeout: 100,
            stdio: 'inherit',
            shell: true,
        }

        exec('gcc random.c -o test', (error, stdout, stderr) => {
            exec('./test', options, (error,stdout,stderr)=>{
                console.log(`stdout: ${stdout}`);
                console.log(`stderr: ${stderr}`);
                res.send("C code logged this in the console - "+stdout);
                if (error) {
                    console.error(`exec error: ${error}`);
                    return;
                }
            });  
            });
});

app.post('/fetchMethods', function(req, res)
{
    console.log("received request for fetch permission");
    var perm=req.body.perm;
    //perm=JSON.stringify(perm);
    console.log("perm sent = "+perm);
    ans={};

    if(perm==null || perm==undefined || perm.length==0)
    {
        res.send("perm not specified");
    }

    else
    {
        var loopCount=0;
        for(i=0;i<perm.length;i++)
        {
            p=perm[i];
            console.log("p= "+p);
            gather_methods(p).then(a=>
                {
                    ans[p]=a;

                    loopCount++;

                    if(loopCount==perm.length)
                    {
                        res.send(ans);
                    }

                }).catch(err=>
                    {
                        console.log("p when err = "+p);
                        res.send("Something went wrong! Try again! "+err);
                    });
        }
    }  
});

app.get('/singlePermissionMethods', function(req, res)
{
    console.log("received request");
    var perm=req.query.perm;
    console.log("perm sent = "+perm);
    ans=[];

    if(perm==null || perm==undefined || perm.length==0)
    {
        res.send("perm not specified");
    }

    else
    {
        gather_methods(perm).then(ans=>
            {

                res.send(ans);

            }).catch(err=>
                {
                    res.send("Something went wrong! Try again! "+err);
                });
    }  
});

app.get('/getPermissions', function(req, res)
{
    ans=[];
    lineReader.eachLine('Axplore_Permission.txt', (line, last) => 
    { 
        var s=line.split("::");
        if(ans.indexOf(s[1])==-1)
        {
            ans.push(s[1]);
        }
        if(last)
        {
            res.send(ans);
        }
    });
});

function gather_methods(perm)
{
    return new Promise(function(resolve,reject)
    {
    ans=[];
    lineReader.eachLine('Axplore_Permission.txt', (line, last) => 
        { 
            var s=line.split("::");
            if(s[1].toLowerCase().indexOf(perm.toLowerCase())!=-1)
            {
                ans.push(s[0]);
            }
            if(last)
            {
                var t;
                var append_to_ans=false;

                lineReader.eachLine('PScout_Permission.txt', (l, lastRecord) => 
                    { 
                        if(l.indexOf("Permission:")!=-1 && l.toLowerCase().indexOf(perm.toLowerCase())!=-1)
                        {
                            //console.log("found");
                            append_to_ans=true;
                        }
                        else if(l.indexOf("Permission:")!=-1 && l.toLowerCase().indexOf(perm.toLowerCase())==-1)
                        {
                            append_to_ans=false;      
                            //console.log("resetting");
                        }

                        if(append_to_ans==true && l.indexOf("Callers")==-1)
                        {
                            t=l.substring(l.indexOf("<")+1,l.indexOf(">"));
                            //console.log("here ");
                            if(t!="")
                            {
                                var h=t.split(": ");
                                var k=h[1].split(" ");
                                var f=h[0]+k[1]+k[0];
                                if(ans.indexOf(f)==-1)
                                {
                                    ans.push(f);
                                }
                                else
                                {
                                    console.log("Duplicate found! Not appending");
                                }
                            }                           
                        }

                        if(lastRecord)
                        {
                            //if(ans.length>0)
                            resolve(ans);
                            
                        }
                    });

                //res.send(ans);
            }
        }); 
    });
}  


//http://localhost:8888/?perm=CAMERA

port = 8888; //process.env.PORT || 8888
app.listen(port);

//<android.media.IAudioService$Stub$Proxy: void startBluetoothScoVirtualCall(android.os.IBinder)> ()

//<android.provider.Telephony$Sms$Outbox: android.net.Uri addMessage(android.content.ContentResolver,java.lang.String,java.lang.String,java.lang.String,java.lang.Long,boolean,long)>