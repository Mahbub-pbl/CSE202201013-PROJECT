import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';
import 'package:flutter_hooks/flutter_hooks.dart';

class WatchStreamPage extends HookWidget {
  WatchStreamPage({super.key, required this.title, required this.ip});
  final String title, ip;
  bool flash_on = false, detect_on = false;
  final _connect = GetConnect();
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
                      _connect.get("http://$ip/stream");
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
                      if (detect_on) {
                        final response = await _connect
                            .get("http://$ip/detect?var=D1&val=0");
                        detect_on = false;
                      } else {
                        await _connect.get("http://$ip/detect?var=D1&val=1");
                        detect_on = true;
                      }
                    },
                    child: const Text('Detect Face',
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
