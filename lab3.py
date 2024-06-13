import threading
import queue
import time
import random
import string

class SpecialMessage:
    def __init__(self, content):
        self.content = content
        self.read_count = 0
        self.lock = threading.Lock()
        self.read_consumer = []

    def increment_read_count(self):
        with self.lock:
            self.read_count += 1

    def is_read_limit_reached(self):
        with self.lock:
            return self.read_count >= 3

class CommunicationBuffer:
    def __init__(self, max_size=10):
        self.queue = queue.Queue(max_size)
        self.special_messages = []
        self.special_lock = threading.Lock()
        self.condition = threading.Condition()
        self.max_size = max_size

    def produce(self, message, is_special=False, special_m=None):
        with self.condition:
            if is_special:
                special_message = special_m
                self.special_messages.append(special_message)
            else:
                while self.queue.full():
                    self.condition.wait()
                self.queue.put(message)
            self.condition.notify_all()

    def try_consume(self, consumer_id, timeout=0.1):
        with self.condition:
            if self.special_messages:
                with self.special_lock:
                    for i in range(0, len(self.special_messages)):
                        special_message = self.special_messages[i]
                        if consumer_id not in special_message.read_consumer:
                            continue
                        else:
                            try:
                                message = self.queue.get(timeout=timeout)
                                self.condition.notify_all()
                                return (message, False)
                            except queue.Empty:
                                return (None, False)
                    if not special_message.is_read_limit_reached():
                        special_message.increment_read_count()
                        special_message.read_consumer.append(consumer_id)
                        return (special_message.content, True)
                    else:
                        self.special_messages.pop(0)
                        if not self.special_messages:
                            return (None, False)
                        special_message = self.special_messages[0]
                        special_message.increment_read_count()
                        return (special_message.content, True)
            try:
                message = self.queue.get(timeout=timeout)
                self.condition.notify_all()
                return (message, False)
            except queue.Empty:
                return (None, False)

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
                special_m = SpecialMessage(message)
                for buffer in self.buffers:
                    buffer.produce(message, is_special=True, special_m=special_m)
            elif self.produce_type == 'mixed':
                message_type = random.choice(['normal', 'special', 'normal', 'normal'])
                if message_type == 'normal':
                    message = random.choice(string.ascii_letters)
                    buffer = random.choice(self.buffers)
                    buffer.produce(message, is_special=False)
                else:
                    message = f'S{random.randint(1, 100)}'
                    special_m = SpecialMessage(message)
                    for buffer in self.buffers:
                        buffer.produce(message, is_special=True, special_m=special_m)
            time.sleep(1)  # Producing message every 1 second

    def stop(self):
        self.running = False

class Consumer(threading.Thread):
    def __init__(self, buffers, consumer_id, read_delay):
        super().__init__()
        self.buffers = buffers
        self.consumer_id = consumer_id
        self.read_delay = read_delay
        self.running = True

    def run(self):
        while self.running:
            for buffer in self.buffers:
                message, is_special = buffer.try_consume(self.consumer_id)
                if message:
                    print(f"Consumer {self.consumer_id} read message: {message} from buffer {id(buffer)} (special: {is_special})")
                time.sleep(self.read_delay)

    def stop(self):
        self.running = False

# Testy i uruchomienie systemu
if __name__ == "__main__":
    buffers = [CommunicationBuffer() for _ in range(3)]



    print("""

          TEST - Single producer producing one special message
          followed by six normal messages
          and a single consumer trying to read these messages.
          """)

    producer = Producer(buffers, 0, 'mixed')

    # Starting the producer for a special message
    special_message = f'S{random.randint(1, 100)}'
    special_m = SpecialMessage(special_message)
    for buffer in buffers:
        buffer.produce(special_message, is_special=True, special_m=special_m)

    # Producing six normal messages
    for _ in range(6):
        normal_message = random.choice(string.ascii_letters)
        buffer = random.choice(buffers)
        buffer.produce(normal_message, is_special=False)

    # Starting the consumer
    consumers_s = [Consumer(buffers, i, random.uniform(0.1, 0.5)) for i in range(2)]
    for consumer in consumers_s:
        consumer.start()

    # Allow some time for the consumer to process the messages
    time.sleep(3)

    # Stopping the consumer
    for consumer in consumers_s:
        consumer.stop()
    for consumer in consumers_s:
        consumer.join()

    print("""

          TEST 1 - po 1 wiadomości zwykłej dla każdego bufora
          od każdego z 5 producentów - 15 wiadomości

          """)

    # Faza początkowa - 5 producentów produkuje po 1 wiadomości dla każdego bufora
    initial_producers = [Producer(buffers, i, 'normal', initial_phase=True) for i in range(5)]
    for producer in initial_producers:
        producer.start()
    for producer in initial_producers:
        producer.join()

    # Startujemy konsumentów
    consumers = [Consumer(buffers, i, random.uniform(0.1, 0.5)) for i in range(8)]
    for consumer in consumers:
        consumer.start()

    # Czekamy aż konsumenci odczytają wszystkie wiadomości zwykłe
    time.sleep(2)

    print("""

          TEST 2 - produkcja 1 wiadomości specjalnej z kopią na każdy bufor.
          Odczytywanie z każdego bufora komunikacyjnego i usunięcie po 3 odczytaniach.

          """)

    # Produkcja jednej wiadomości specjalnej z kopią na każdy bufor
    message = f'S{random.randint(1, 100)}'
    s_m = SpecialMessage(message)
    for buffer in buffers:
        buffer.produce(s_m, is_special=True, special_m=s_m)

    # Czekamy aż wiadomość specjalna zostanie odczytana 3 razy
    time.sleep(3)

    print("""

          TEST 3 - produkcja 1 wiadomości specjalnej z kopią na każdy semafor.
          Tylko 1 bufor komunikacyjny, usunięcie wiadomości po 3 odczytaniach.

          """)

    # Nowy bufor komunikacyjny dla testu 3
    single_buffer = [CommunicationBuffer()]

    single_consumers = [Consumer(single_buffer, i, random.uniform(0.1, 0.5)) for i in range(8)]

    for consumer in single_consumers:
        consumer.start()

    # Produkcja jednej wiadomości specjalnej z kopią na każdy bufor
    message = f'S{random.randint(1, 100)}'
    s_m = SpecialMessage(message)
    for buffer in single_buffer:
        buffer.produce(s_m, is_special=True, special_m=s_m)




    # Czekamy aż wiadomość specjalna zostanie odczytana 3 razy
    time.sleep(3)

    print("""

          TEST 4 - działanie bez przerwy przez 10 sekund

          """)

    # Tworzenie producentów
    continuous_producers = [Producer(buffers, i, 'mixed') for i in range(5)]
    for producer in continuous_producers:
        producer.start()

    # Startowanie konsumentów
    continuous_consumers = [Consumer(buffers, i, random.uniform(0.1, 0.5)) for i in range(8)]
    for consumer in continuous_consumers:
        consumer.start()

    # Praca przez 10 sekund
    time.sleep(10)

    # Zatrzymywanie producentów
    for producer in continuous_producers:
        producer.stop()
    for producer in continuous_producers:
        producer.join()

    # Zatrzymywanie konsumentów
    for consumer in continuous_consumers:
        consumer.stop()
    for consumer in continuous_consumers:
        consumer.join()




    print("Testy zakończone.")
