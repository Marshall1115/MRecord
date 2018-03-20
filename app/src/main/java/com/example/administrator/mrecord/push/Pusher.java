package com.example.administrator.mrecord.push;


import com.example.administrator.mrecord.push.params.Params;

/**
 * Created by Administrator on 2017/5/14.
 */

public abstract class Pusher<T extends Params> {
    protected T parmas;
    protected boolean isPushing;

    public Pusher(T parmas) {
        this.parmas = parmas;
    }

    public boolean isPushing() {
        return isPushing;
    }

    public void setPushing(boolean pushing) {
        isPushing = pushing;
    }

    abstract void prepare();

    public boolean statPush() {
        setPushing (true);
        //开始录音
        return true;
    }

    public boolean stopPush() {
        setPushing (false);
        return true;
    }

    abstract boolean release();
}
