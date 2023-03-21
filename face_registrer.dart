import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';
import 'package:fluttertoast/fluttertoast.dart';
import 'package:get/get_connect/connect.dart';
import 'package:video_player/video_player.dart';
import 'package:flutter_hooks/flutter_hooks.dart';

class FaceRegisterPage extends HookWidget {
  FaceRegisterPage({super.key, required this.title, required this.ip});
  final String title, ip;
  final TextEditingController tEC1 = TextEditingController();
  final _connect = GetConnect();
  bool flash_on = false;
  late VideoPlayerController _controller;

  @override
  Widget build(BuildContext context) {
    final isRunning = useState(true);
    _connect.get("http://$ip/recog?var=frecog&val=0");
    return Scaffold(
        appBar: AppBar(
          title: Text(title),
        ),
        body: Center(
          child: SingleChildScrollView(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: <Widget>[
                SizedBox(
                  width: 355,
                  child: Mjpeg(
                    isLive: isRunning.value,
                    fit: BoxFit.fill,
                    error: (context, error, stack) {
                      print(error);
                      print(stack);
                      return Text(error.toString(),
                          style: const TextStyle(color: Colors.red));
                    },
                    stream: "http://$ip:81/stream",
                  ),
                ),
                SizedBox(
                    height: 60,
                    width: 300,
                    child: TextFormField(
                      controller: tEC1,
                      style: const TextStyle(
                        fontSize: 18,
                        color: Colors.black,
                      ),
                      decoration: const InputDecoration(
                          hintText: "Person Name",
                          hintStyle: TextStyle(
                            //fontWeight: Colors.a,
                            fontSize: 18,
                            color: Colors.grey,
                          )),
                    )),
                const SizedBox(height: 15),
                SizedBox(
                    height: 40,
                    width: 200,
                    child: ElevatedButton(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.deepPurpleAccent,
                          elevation: 5,
                        ), // foreground
                        onPressed: () async {
                          if (flash_on) {
                            final response = await _connect
                                .get("http://$ip/led?var=flash&val=0");
                            flash_on = false;
                          } else {
                            await _connect
                                .get("http://$ip/led?var=flash&val=1");
                            flash_on = true;
                          }
                        },
                        child: const Text('Flash',
                            style: TextStyle(
                              fontWeight: FontWeight.w900,
                              fontSize: 18,
                              color: Colors.white,
                            )))),
                const SizedBox(height: 15),
                SizedBox(
                    height: 40,
                    width: 200,
                    child: ElevatedButton(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.deepPurpleAccent,
                          elevation: 5,
                        ),
                        onPressed: () async {
                          if (tEC1.text == "") {
                            Fluttertoast.showToast(
                                msg: "Input Name",
                                toastLength: Toast.LENGTH_SHORT,
                                gravity: ToastGravity.BOTTOM,
                                timeInSecForIosWeb: 1,
                                backgroundColor: const Color(0xFF82A30A),
                                textColor: const Color(0xFF0D1141),
                                fontSize: 16);
                          } else {
                            final response = await _connect.get(
                                "http://$ip/enroll?var=${tEC1.text}&val=1");
                            Timer.periodic(const Duration(seconds: 1),
                                (timer) async {
                              final resp = await _connect
                                  .get("http://$ip/rc?var=rc&val=1");
                              String? a = resp.bodyString;
                              a = a ?? "";
                              if (a.contains("enc")) {
                                Fluttertoast.showToast(
                                    msg: "Face Registration Completed",
                                    toastLength: Toast.LENGTH_LONG,
                                    gravity: ToastGravity.TOP,
                                    timeInSecForIosWeb: 1,
                                    backgroundColor: const Color(0xFF82A30A),
                                    textColor: const Color(0xFF0D1141),
                                    fontSize: 16);
                                isRunning.value = !isRunning.value;
                                timer.cancel();
                              }
                            });
                          }
                        },
                        child: const Text('Register Face',
                            style: TextStyle(
                              fontWeight: FontWeight.w900,
                              fontSize: 18,
                              color: Colors.white,
                            )))),
              ],
            ),
          ),
        ));
  }
}
