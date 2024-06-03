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

    def produce(self, message, is_special=False):
        with self.condition:
            if is_special:
                special_message = SpecialMessage(message)
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
                    special_message = self.special_messages[0]
                    if not special_message.is_read_limit_reached():
                        special_message.increment_read_count()
                        return (special_message.content, True)
                    else:
                        self.special_messages.pop(0)
                        return (None, False)
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
                for buffer in buffers:
                    buffer.produce(message, is_special=True)
            elif self.produce_type == 'mixed':
                message_type = random.choice(['normal', 'special', 'normal', 'normal'])
                if message_type == 'normal':
                    message = random.choice(string.ascii_letters)
                else:
                    message = f'S{random.randint(1, 100)}'
                for buffer in buffers:
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
    for buffer in buffers:
        buffer.produce("Special Message", is_special=True)

    # Startujemy konsumentów
    consumers = [Consumer(buffers, i, random.uniform(0.1, 0.5)) for i in range(8)]
    for consumer in consumers:
        consumer.start()

    # Czekamy aż wiadomość specjalna zostanie odczytana 3 razy
    time.sleep(3)

    print("""

          TEST 3 - produkcja 1 wiadomości specjalnej z kopią na każdy semafor.
          Tylko 1 bufor komunikacyjny, usunięcie wiadomości po 3 odczytaniach.

          """)

    # Nowy bufor komunikacyjny dla testu 3
    single_buffer = [CommunicationBuffer()]

    # Produkcja jednej wiadomości specjalnej z kopią na każdy bufor
    for buffer in single_buffer:
        buffer.produce("Special Message", is_special=True)

    # Startujemy konsumentów
    consumers = [Consumer(single_buffer, i, random.uniform(0.1, 0.5)) for i in range(8)]
    for consumer in consumers:
        consumer.start()

    # Czekamy aż wiadomość specjalna zostanie odczytana 3 razy
    time.sleep(3)

    print("Testy zakończone.")
