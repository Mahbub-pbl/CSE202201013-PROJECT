import 'package:flutter/material.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:video_player/video_player.dart';
import 'package:get/get.dart';

class FaceRecognitionPage extends HookWidget {
  FaceRecognitionPage({super.key, required this.title, required this.ip});
  final String title, ip;
  bool flash_on = false, recog_on = false;
  final _connect = GetConnect();
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
                        await _connect.get("http://$ip/led?var=flash&val=1");
                        flash_on = true;
                      }
                    },
                    child: const Text('Flash',
                        style: TextStyle(
                          fontWeight: FontWeight.w900,
                          fontSize: 20,
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
                    ), // foreground
                    onPressed: () async {
                      if (recog_on) {
                        final response = await _connect
                            .get("http://$ip/recog?var=frecog&val=0");
                        recog_on = false;
                      } else {
                        await _connect.get("http://$ip/recog?var=frecog&val=1");
                        recog_on = true;
                      }
                    },
                    child: const Text('Recognize Face',
                        style: TextStyle(
                          fontWeight: FontWeight.w900,
                          fontSize: 20,
                          color: Colors.white,
                        ))))
          ],
        ),
      ),
    );
  }
}
