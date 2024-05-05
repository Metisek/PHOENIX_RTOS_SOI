import threading
import random
import time
from collections import deque

class Semaphore(threading._Semaphore):
    wait = threading._Semaphore.acquire
    signal = threading._Semaphore.release

class Buffer:
    def __init__(self, size):
        self.size = size
        self.buffer = deque()
        self.mutex = Semaphore(1)
        self.items = Semaphore(0)
        self.spaces = Semaphore(size)

    def produce(self, item):
        self.spaces.wait()
        self.mutex.wait()
        self.buffer.append(item)
        self.mutex.signal()
        self.items.signal()

    def consume(self):
        self.items.wait()
        self.mutex.wait()
        item = self.buffer.popleft()
        self.mutex.signal()
        self.spaces.signal()
        return item

class Producer(threading.Thread):
    def __init__(self, id, buffer):
        super().__init__()
        self.id = id
        self.buffer = buffer

    def run(self):
        while True:
            message = self.generate_message()
            self.buffer.produce(message)
            time.sleep(random.uniform(0.1, 0.5))

    def generate_message(self):
        message_type = "S" if random.random() < 0.1 else ""
        message_id = random.randint(1, 99)
        return f"{self.id:02d}{message_type}{message_id}"

class Consumer(threading.Thread):
    def __init__(self, id, buffer):
        super().__init__()
        self.id = id
        self.buffer = buffer

    def run(self):
        while True:
            message = self.buffer.consume()
            self.process_message(message)

    def process_message(self, message):
        print(f"Consumer {self.id} processed message: {message}")

def main():
    buffer_size = 10
    special_buffer_size = 8
    buffer = Buffer(buffer_size)
    special_buffer = Buffer(special_buffer_size)

    producers = []
    consumers = []

    for i in range(5):
        producer = Producer(i + 1, buffer)
        producers.append(producer)
        producer.start()

    for i in range(8):
        consumer = Consumer(i + 1, buffer if i % 2 == 0 else special_buffer)
        consumers.append(consumer)
        consumer.start()

    for producer in producers:
        producer.join()

    for consumer in consumers:
        consumer.join()

if __name__ == "__main__":
    main()
