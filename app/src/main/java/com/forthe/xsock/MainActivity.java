package com.forthe.xsock;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;


public class MainActivity extends AppCompatActivity implements View.OnClickListener{

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
    private InfoAdapter adapter;
    private TextView tv_ip;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        findViewById(R.id.tv_start_server).setOnClickListener(this);
        findViewById(R.id.send_broadcast).setOnClickListener(this);
        findViewById(R.id.start_client).setOnClickListener(this);

        ListView lv = (ListView) findViewById(R.id.lv_infos);

        adapter = new InfoAdapter(this);
        lv.setAdapter(adapter);


        tv_ip = (TextView) findViewById(R.id.tv_ip);
        tv_ip.setText(getIp());

        handler = new Handler();
        handler.postDelayed(runnable,250);
    }

    class InfoAdapter extends Adapter<String>{

        public InfoAdapter(Context mContext) {
            super(mContext);
        }

        @Override
        public View getView(int i, View view, ViewGroup viewGroup) {
            TextView tv;
            if(view == null){
                tv = new TextView(getContext());
                tv.setTextSize(12f);
                tv.setTextColor(0xffffffff);
                tv.setLines(1);
                view = tv;
            }else{
                tv = (TextView) view;
            }
            tv.setText(getItem(i));
            return view;
        }
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */

    public native void getOnLog();
    public native void broadcastMsg(String sendMsg);
    public native void startServer(String name);
    public native void startClient();
    private Handler handler;
    public void onLog(String log){
        adapter.addData(log);
    }

    private Runnable runnable = new Runnable() {
        @Override
        public void run() {
            getOnLog();
            handler.postDelayed(runnable, 250);
        }
    };

    private int count = 0;
    @Override
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.tv_start_server:
                new Thread(){
                    @Override
                    public void run() {
                        startServer("test");
                    }
                }.start();
                break;
            case R.id.send_broadcast:
                count++;
                broadcastMsg("test send broadcast "+count);
                break;
            case R.id.start_client:
                new Thread(){
                    @Override
                    public void run() {
                        startClient();
                    }
                }.start();
                break;
        }
    }


    private String getIp(){
        //获取wifi服务
        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        //判断wifi是否开启
        if (!wifiManager.isWifiEnabled()) {
            wifiManager.setWifiEnabled(true);
        }
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        int ip = wifiInfo.getIpAddress();
        return (ip & 0xFF ) + "." +
                ((ip >> 8 ) & 0xFF) + "." +
                ((ip >> 16 ) & 0xFF) + "." +
                ( ip >> 24 & 0xFF) ;
    }
}
