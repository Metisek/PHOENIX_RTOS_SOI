import threading
import queue
import time
import random
import string

class CommunicationBufferMonitor:
    def __init__(self, max_size=10):
        self.queue = queue.Queue(max_size)
        self.special_messages = []
        self.special_lock = threading.Lock()
        self.condition = threading.Condition()
        self.max_size = max_size
        self.special_size_limit = 5
        self.special_acknowledged = set()

    def produce(self, message, is_special=False):
        with self.condition:
            if is_special:
                with self.special_lock:
                    self.special_messages.append(message)
                    if len(self.special_messages) > self.special_size_limit:
                        self.special_messages = self.special_messages[-self.special_size_limit:]
            else:
                while self.queue.full():
                    self.condition.wait()
                self.queue.put(message)
            self.condition.notify_all()

    def try_consume(self, consumer_id, timeout=0.1):
        with self.condition:
            if self.special_messages:
                with self.special_lock:
                    message = self.special_messages[0]
                return (message, True)
            try:
                message = self.queue.get(timeout=timeout)
                self.condition.notify_all()
                return (message, False)
            except queue.Empty:
                return (None, False)

    def acknowledge_special(self, consumer_id):
        with self.special_lock:
            self.special_acknowledged.add(consumer_id)
            if len(self.special_acknowledged) == 8:  # All consumers have read the message
                self.special_messages.pop(0)
                self.special_acknowledged.clear()


class Producer(threading.Thread):
    def __init__(self, buffers, producer_id, produce_type, initial_phase=False):
        super().__init__()
        self.buffers = buffers
        self.producer_id = producer_id
        self.produce_type = produce_type
        self.initial_phase = initial_phase
        self.running = True

    def run(self):
        if self.initial_phase:
            for buffer in self.buffers:
                message = random.choice(string.ascii_letters)
                buffer.produce(message, is_special=False)
            return

        while self.running:
            if self.produce_type == 'normal':
                message = random.choice(string.ascii_letters)
                buffer = random.choice(self.buffers)
                buffer.produce(message, is_special=False)
            elif self.produce_type == 'special':
                message = f'S{random.randint(1, 100)}'
                buffer = random.choice(self.buffers)
                buffer.produce(message, is_special=True)
            elif self.produce_type == 'mixed':
                message_type = random.choice(['normal', 'special', 'normal', 'normal'])
                if message_type == 'normal':
                    message = random.choice(string.ascii_letters)
                else:
                    message = f'S{random.randint(1, 100)}'
                buffer = random.choice(self.buffers)
                buffer.produce(message, is_special=(message_type == 'special'))
            time.sleep(1)  # Producing message every 1 second

    def stop(self):
        self.running = False


class Consumer(threading.Thread):
    def __init__(self, buffers, consumer_id, read_delay):
        super().__init__()
        self.buffers = buffers
        self.consumer_id = consumer_id
        self.read_delay = read_delay
        self.read_special = {id(buffer): False for buffer in buffers}
        self.running = True

    def run(self):
        while self.running:
            for buffer in self.buffers:
                message, is_special = buffer.try_consume(self.consumer_id)
                if message:
                    print(f"Consumer {self.consumer_id} read message: {message} from buffer {id(buffer)} (special: {is_special})")
                    if is_special:
                        self.read_special[id(buffer)] = True
                        buffer.acknowledge_special(self.consumer_id)
                time.sleep(self.read_delay)

    def stop(self):
        self.running = False


def change_production(producers, new_type):
    for producer in producers:
        producer.produce_type = new_type


# Test and launch the system
if __name__ == "__main__":
    buffers = [CommunicationBufferMonitor() for _ in range(3)]

    print("""

          TEST 1 - one normal message per buffer
          from each of 5 producers - 15 messages in total

          """)

    # Initial phase - 5 producers produce one message for each buffer
    initial_producers = [Producer(buffers, i, 'normal', initial_phase=True) for i in range(5)]
    for producer in initial_producers:
        producer.start()
    for producer in initial_producers:
        producer.join()

    # Start consumers
    consumers = [Consumer(buffers, i, random.uniform(0.1, 0.5)) for i in range(8)]
    for consumer in consumers:
        consumer.start()

    # Wait for consumers to read all normal messages
    time.sleep(2)

    print("""

          TEST 2 - one special message, read by every consumer

          """)

    # Produce one special message by a random producer
    special_producer = Producer(buffers, 0, 'special', initial_phase=False)
    special_producer.start()
    time.sleep(1)
    special_producer.stop()
    special_producer.join()

    # Wait for consumers to read the special message
    time.sleep(3)

    print("""

          TEST 3 - Mixed messages, no limit

          """)

    # Start normal tests for 5 seconds
    producers = [Producer(buffers, i, 'mixed') for i in range(5)]
    for producer in producers:
        producer.start()

    time.sleep(5)

    # Stop producers and consumers
    for producer in producers:
        producer.stop()
    for producer in producers:
        producer.join()

    for consumer in consumers:
        consumer.stop()
    for consumer in consumers:
        consumer.join()

    print("Test concluded.")
